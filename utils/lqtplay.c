/*
 * simple quicktime movie player, needs libquicktime (or quicktime4linux).
 *
 *  (c) 2002 Gerd Knorr <kraxel@bytesex.org>
*
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <errno.h>
#include <endian.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>

#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xaw/Simple.h>
#include <X11/extensions/XShm.h>

#include <quicktime/quicktime.h>

/* ------------------------------------------------------------------------ */
/* X11 code                                                                 */

static XtAppContext app_context;
static Widget       app_shell,simple;
static Display      *dpy;
static Visual       *visual;
static XVisualInfo  vinfo,*vinfo_list;
static XPixmapFormatValues *pf;

static int no_mitshm = 0;
static int pixmap_bytes = 0;
static int x11_byteswap = 0;

static unsigned long   lut_red[256];
static unsigned long   lut_green[256];
static unsigned long   lut_blue[256];

#define SWAP2(x) (((x>>8) & 0x00ff) |\
                  ((x<<8) & 0xff00))

#define SWAP4(x) (((x>>24) & 0x000000ff) |\
                  ((x>>8)  & 0x0000ff00) |\
                  ((x<<8)  & 0x00ff0000) |\
                  ((x<<24) & 0xff000000))

static void
x11_lut(unsigned long red_mask, unsigned long green_mask,
	unsigned long blue_mask, int bytes, int swap)
{
    int             rgb_red_bits = 0;
    int             rgb_red_shift = 0;
    int             rgb_green_bits = 0;
    int             rgb_green_shift = 0;
    int             rgb_blue_bits = 0;
    int             rgb_blue_shift = 0;
    unsigned int    i;
    unsigned int    mask;

    for (i = 0; i < 32; i++) {
        mask = (1 << i);
        if (red_mask & mask)
            rgb_red_bits++;
        else if (!rgb_red_bits)
            rgb_red_shift++;
        if (green_mask & mask)
            rgb_green_bits++;
        else if (!rgb_green_bits)
            rgb_green_shift++;
        if (blue_mask & mask)
            rgb_blue_bits++;
        else if (!rgb_blue_bits)
            rgb_blue_shift++;
    }
    if (rgb_red_bits > 8)
	for (i = 0; i < 256; i++)
	    lut_red[i] = (i << (rgb_red_bits + rgb_red_shift - 8));
    else
	for (i = 0; i < 256; i++)
	    lut_red[i] = (i >> (8 - rgb_red_bits)) << rgb_red_shift;
    if (rgb_green_bits > 8)
	for (i = 0; i < 256; i++)
	    lut_green[i] = (i << (rgb_green_bits + rgb_green_shift - 8));
    else
	for (i = 0; i < 256; i++)
	    lut_green[i] = (i >> (8 - rgb_green_bits)) << rgb_green_shift;
    if (rgb_blue_bits > 8)
	for (i = 0; i < 256; i++)
	    lut_blue[i] = (i << (rgb_blue_bits + rgb_blue_shift - 8));
    else
	for (i = 0; i < 256; i++)
	    lut_blue[i] = (i >> (8 - rgb_blue_bits)) << rgb_blue_shift;

    if (2 == bytes && swap) {
	for (i = 0; i < 256; i++) {
	    lut_red[i] = SWAP2(lut_red[i]);
	    lut_green[i] = SWAP2(lut_green[i]);
	    lut_blue[i] = SWAP2(lut_blue[i]);
	}
    }
    if (4 == bytes && swap) {
	for (i = 0; i < 256; i++) {
	    lut_red[i] = SWAP4(lut_red[i]);
	    lut_green[i] = SWAP4(lut_green[i]);
	    lut_blue[i] = SWAP4(lut_blue[i]);
	}
    }
}

static void
rgb_to_lut2(unsigned char *dest, unsigned char *src, int p)
{
    unsigned short *d = (unsigned short*)dest;

    while (p-- > 0) {
	*(d++) = lut_red[src[0]] | lut_green[src[1]] | lut_blue[src[2]];
	src += 3;
    }
}

static void
rgb_to_lut4(unsigned char *dest, unsigned char *src, int p)
{
    unsigned int *d = (unsigned int*)dest;

    while (p-- > 0) {
	*(d++) = lut_red[src[0]] | lut_green[src[1]] | lut_blue[src[2]];
	src += 3;
    }
}

static void x11_init(void)
{
    int i,n;
    
    /* get visual info */
    visual = DefaultVisualOfScreen(XtScreen(app_shell));
    vinfo.visualid = XVisualIDFromVisual(visual);
    vinfo_list = XGetVisualInfo(dpy, VisualIDMask, &vinfo, &n);
    vinfo = vinfo_list[0];
    if (vinfo.class != TrueColor || vinfo.depth < 15) {
	fprintf(stderr,"can't handle visual\n");
	exit(1);
    }

    /* look for default pixmap format */
    pf = XListPixmapFormats(dpy,&n);
    for (i = 0; i < n; i++)
	if (pf[i].depth == vinfo.depth)
	    pixmap_bytes = pf[i].bits_per_pixel/8;

    /* byteswapping needed ??? */
    if (ImageByteOrder(dpy)==LSBFirst && BYTE_ORDER!=LITTLE_ENDIAN)
	x11_byteswap=1;
    if (ImageByteOrder(dpy)==MSBFirst && BYTE_ORDER!=BIG_ENDIAN)
	x11_byteswap=1;

    /* init lookup tables */
    x11_lut(vinfo.red_mask, vinfo.green_mask, vinfo.blue_mask,
	    pixmap_bytes,x11_byteswap);
}

static int
catch_no_mitshm(Display * dpy, XErrorEvent * event)
{
    no_mitshm++;
    return 0;
}

static XImage*
x11_create_ximage(Display *dpy, int width, int height)
{
    XImage          *ximage = NULL;
    unsigned char   *ximage_data;
    XShmSegmentInfo *shminfo = NULL;
    void            *old_handler;
    
    if (no_mitshm)
	goto no_mitshm;
    
    old_handler = XSetErrorHandler(catch_no_mitshm);
    shminfo = malloc(sizeof(XShmSegmentInfo));
    memset(shminfo, 0, sizeof(XShmSegmentInfo));
    ximage = XShmCreateImage(dpy,vinfo.visual,vinfo.depth,
			     ZPixmap, NULL,
			     shminfo, width, height);
    if (NULL == ximage)
	goto shm_error;
    shminfo->shmid = shmget(IPC_PRIVATE,
			    ximage->bytes_per_line * ximage->height,
			    IPC_CREAT | 0777);
    if (-1 == shminfo->shmid) {
	perror("shmget");
	goto shm_error;
    }
    shminfo->shmaddr = (char *) shmat(shminfo->shmid, 0, 0);
    if ((void *)-1 == shminfo->shmaddr) {
	perror("shmat");
	goto shm_error;
    }
    ximage->data = shminfo->shmaddr;
    shminfo->readOnly = False;
    
    XShmAttach(dpy, shminfo);
    XSync(dpy, False);
    if (no_mitshm)
	goto shm_error;
    shmctl(shminfo->shmid, IPC_RMID, 0);
    XSetErrorHandler(old_handler);

shm_error:
    if (ximage) {
	XDestroyImage(ximage);
	ximage = NULL;
    }
    if ((void *)-1 != shminfo->shmaddr  && NULL != shminfo->shmaddr)
	shmdt(shminfo->shmaddr);
    free(shminfo);
    XSetErrorHandler(old_handler);
    no_mitshm = 1;

 no_mitshm:
    if (NULL == (ximage_data = malloc(width * height * pixmap_bytes))) {
	fprintf(stderr,"out of memory\n");
	exit(1);
    }
    ximage = XCreateImage(dpy, vinfo.visual, vinfo.depth,
			  ZPixmap, 0, ximage_data,
			  width, height,
			  8, 0);
    memset(ximage->data, 0, ximage->bytes_per_line * ximage->height);
    return ximage;
}

/* ------------------------------------------------------------------------ */
/* oss code                                                                 */

static int oss_fd = -1;

static int
oss_setformat(int chan, int rate)
{
    int hw_afmt = AFMT_S16_NE;
    int hw_chan = chan;
    int hw_rate = rate;

    ioctl(oss_fd, SNDCTL_DSP_SETFMT, &hw_afmt);
    if (AFMT_S16_LE != hw_afmt) {
	fprintf(stderr,"ERROR: can't set sound format\n");
	exit(1);
    }
    ioctl(oss_fd, SNDCTL_DSP_CHANNELS, &hw_chan);
    if (chan != hw_chan) {
	fprintf(stderr,"ERROR: can't set sound channels\n");
	exit(1);
    }
    ioctl(oss_fd, SNDCTL_DSP_SPEED, &hw_rate);
    if (rate != hw_rate) {
	fprintf(stderr,"WARNING: sample rate mismatch (need %d, got %d)\n",
		rate,hw_rate);
    }
    return 0;
}

static int oss_init(char *dev, int channels, int rate)
{
    int trigger;

    oss_fd = open(dev,O_WRONLY | O_NONBLOCK);
    if (-1 == oss_fd) {
	fprintf(stderr,"open %s: %s\n",dev,strerror(errno));
	return -1;
    }
    oss_setformat(channels,rate);
    trigger = PCM_ENABLE_OUTPUT;
    ioctl(oss_fd,SNDCTL_DSP_SETTRIGGER,&trigger);
    return 0;
}

/* ------------------------------------------------------------------------ */
/* quicktime code                                                           */

static quicktime_t *qt;
static int qt_hasvideo,qt_hasaudio;

static int qt_width = 320, qt_height = 32, qt_drop, qt_droptotal;
static unsigned char *qt_frame,**qt_rows;
static XImage *qt_ximage;
static GC qt_gc;

static int16_t *qt_audio,*qt1,*qt2;
static int qt_size,qt_offset,qt_stereo;

static void qt_init(FILE *fp, char *filename)
{
    char *str;
    int i;

    /* open file */
    qt = quicktime_open(filename,1,0);
    if (NULL == qt) {
	fprintf(fp,"can't open file: %s\n",filename);
	exit(1);
    }

    /* print misc info */
    fprintf(fp,"going to play %s\n",filename);
    str = quicktime_get_copyright(qt);
    if (str)
	fprintf(fp,"  copyright: %s\n",str);
    str = quicktime_get_name(qt);
    if (str)
	fprintf(fp,"  name: %s\n",str);
    str = quicktime_get_info(qt);
    if (str)
	fprintf(fp,"  info: %s\n",str);
    
    /* print video info */
    if (quicktime_has_video(qt)) {
	fprintf(fp,"  video: %d track(s)\n",quicktime_video_tracks(qt));
	for (i = 0; i < quicktime_video_tracks(qt); i++) {
	    fprintf(fp,
		    "    track #%d\n"
		    "      width : %d\n"
		    "      height: %d\n"
		    "      depth : %d bit\n"
		    "      rate  : %.2f fps\n"
		    "      codec : %s\n",
		    i+1,
		    quicktime_video_width(qt,i),
		    quicktime_video_height(qt,i),
		    quicktime_video_depth(qt,i),
		    quicktime_frame_rate(qt,i),
		    quicktime_video_compressor(qt,i));
	}
    }

    /* print audio info */
    if (quicktime_has_audio(qt)) {
	fprintf(fp,"  audio: %d track(s)\n",quicktime_audio_tracks(qt));
	for (i = 0; i < quicktime_audio_tracks(qt); i++) {
	    fprintf(fp,
		    "    track #%d\n"
		    "      rate  : %ld Hz\n"
		    "      bits  : %d\n"
		    "      chans : %d\n"
		    "      codec : %s\n",
		    i+1,
		    quicktime_sample_rate(qt,i),
		    quicktime_audio_bits(qt,i),
		    quicktime_track_channels(qt,i),
		    quicktime_audio_compressor(qt,i));
	}
    }

    /* sanity checks */
    if (!quicktime_has_video(qt)) {
	fprintf(stderr,"WARNING: no video stream\n");
    } else if (!quicktime_supported_video(qt,0)) {
	fprintf(stderr,"WARNING: unsupported video codec\n");
    } else {
	qt_hasvideo = 1;
	qt_width  = quicktime_video_width(qt,0);
	qt_height = quicktime_video_height(qt,0);
    }

    if (!quicktime_has_audio(qt)) {
	fprintf(stderr,"WARNING: no audio stream\n");
    } else if (!quicktime_supported_audio(qt,0)) {
	fprintf(stderr,"WARNING: unsupported audio codec\n");
    } else {
	qt_hasaudio = 1;
	if (quicktime_track_channels(qt,0) > 1)
	    qt_stereo = 1;
	if (-1 == oss_init("/dev/dsp", qt_stereo ? 2 : 1,
			   quicktime_sample_rate(qt,0)))
	    qt_hasaudio = 0;
    }

    if (0 == qt_hasvideo && 0 == qt_hasaudio) {
	fprintf(stderr,"ERROR: no playable stream found\n");
	exit(1);
    }
}

static int qt_frame_blit(void)
{
    int i;
    
    if (0 == quicktime_video_position(qt,0)) {
	/* init */
	qt_frame  = malloc(qt_width * qt_height * 4);
	qt_rows   = malloc(qt_height * sizeof(char*));
	for (i = 0; i < qt_height; i++)
	    qt_rows[i] = qt_frame + qt_width * 3 * i;
	qt_ximage = x11_create_ximage(dpy,qt_width,qt_height);
	qt_gc = XCreateGC(dpy,XtWindow(simple),0,NULL);
    }

    if (qt_drop) {
	qt_droptotal += qt_drop;
	fprintf(stderr,"dropped %d frame(s)\r",qt_droptotal);
	for (i = 0; i < qt_drop; i++)
	    quicktime_read_frame(qt,qt_frame,0);
	qt_drop = 0;
    }
    quicktime_decode_video(qt,qt_rows,0);
    switch (pixmap_bytes) {
    case 2:
	rgb_to_lut2(qt_ximage->data,qt_frame,qt_width*qt_height);
	break;
    case 4:
	rgb_to_lut4(qt_ximage->data,qt_frame,qt_width*qt_height);
	break;
    }
    if (no_mitshm)
	XPutImage(dpy,XtWindow(simple),qt_gc,qt_ximage,
		  0,0,0,0, qt_width,qt_height);
    else
	XShmPutImage(dpy,XtWindow(simple),qt_gc,qt_ximage,
		     0,0,0,0, qt_width,qt_height, True);

    if (quicktime_video_position(qt,0) >= quicktime_video_length(qt,0))
	return -1;
    return 0;
}

static void qt_frame_delay(struct timeval *start, struct timeval *wait)
{
    struct timeval now;
    long msec;

    gettimeofday(&now,NULL);
    msec  = (start->tv_sec  - now.tv_sec)  * 1000;
    msec += (start->tv_usec - now.tv_usec) / 1000;
    msec += quicktime_video_position(qt,0) * 1000
	/ quicktime_frame_rate(qt,0);
    if (msec < 0) {
	qt_drop = -msec * quicktime_frame_rate(qt,0) / 1000;
	wait->tv_sec  = 0;
	wait->tv_usec = 0;
    } else {
	wait->tv_sec  = msec / 1000;
	wait->tv_usec = (msec % 1000) * 1000;
    }
}

static int qt_audio_write(void)
{
    long pos;
    int rc,i;

    if (0 == quicktime_audio_position(qt,0)) {
	/* init */
	qt_size   = 64 * 1024;
	qt_offset = 0;
	qt_audio  = malloc(qt_size);
	if (quicktime_track_channels(qt,0) > 1) {
	    qt1 = malloc(qt_size/2);
	    qt2 = malloc(qt_size/2);
	}
    }

    if (0 == qt_offset) {
	if (qt_stereo) {
	    /* stereo: two channels => interlaved samples */
	    pos = quicktime_audio_position(qt,0);
	    quicktime_decode_audio(qt,qt1,NULL,qt_size/4,0);
	    quicktime_set_audio_position(qt,pos,0);
	    quicktime_decode_audio(qt,qt2,NULL,qt_size/4,1);
	    for (i = 0; i < qt_size/4; i++) {
		qt_audio[2*i+0] = qt1[i];
		qt_audio[2*i+1] = qt2[i];
	    }
	} else {
	    /* mono */
	    quicktime_decode_audio(qt,qt_audio,NULL,qt_size/2,0);
	}
    }
    rc = write(oss_fd,qt_audio+qt_offset/2,qt_size-qt_offset);
    switch (rc) {
    case -1:
	perror("write dsp");
	close(oss_fd);
	oss_fd = -1;
	break;
    case 0:
	fprintf(stderr,"write dsp: Huh? no data written?\n");
	close(oss_fd);
	oss_fd = -1;
	break;
    default:
	qt_offset += rc;
	if (qt_offset == qt_size)
	    qt_offset = 0;
    }

    if (quicktime_audio_position(qt,0) >= quicktime_audio_length(qt,0) &&
0 == qt_offset)
	return -1;
    return 0;
}

/* ------------------------------------------------------------------------ */
/* main                                                                     */

static void usage(FILE *fp, char *prog)
{
    char *p;

    p = strrchr(prog,'/');
    if (p) prog = p+1;
    fprintf(fp,
	    "\n"
	    "Very simple quicktime movie player for X11.  Just playes\n"
	    "the movie and nothing else.  No fancy gui, no controls,\n"
	    "no command line switches.  Nothing.  Nada.  All you can\n"
	    "do is quit with 'Q' and 'ESC' keys ...\n"
	    "\n"
	    "usage: %s <file>\n"
	    "\n",
	    prog);
}

static void quit_ac(Widget widget, XEvent *event,
	     String *params, Cardinal *num_params)
{
    exit(0);
}

static XtActionsRec action_table[] = {
    { "Quit",  quit_ac },
};

static String res[] = {
    "lqtplay.playback.translations:  #override \\n"
    "        <Key>Q:                 Quit()    \\n"
    "        <Key>Escape:            Quit()",
    "lqtplay.playback.background:    black",
    NULL
};

int main(int argc, char *argv[])
{
    struct timeval start,wait;
    
    app_shell = XtVaAppInitialize(&app_context, "lqtplay",
				  NULL, 0,
				  &argc, argv,
				  res, NULL);
    if (argc < 2) {
	usage(stderr,argv[0]);
	exit(1);
    }
    qt_init(stdout,argv[1]);

    dpy = XtDisplay(app_shell);
    XtAppAddActions(app_context,action_table,
		    sizeof(action_table)/sizeof(XtActionsRec));
    XtVaSetValues(app_shell, XtNtitle,argv[1],NULL);
    simple = XtVaCreateManagedWidget("playback",simpleWidgetClass,app_shell,
				     XtNwidth,  qt_width,
				     XtNheight, qt_height,
				     NULL);

    x11_init();
    XtRealizeWidget(app_shell);
    
    /* main loop */
    gettimeofday(&start,NULL);
    for (;;) {
	int rc,max;
	fd_set rd,wr;
	XEvent event;

	if (True == XCheckMaskEvent(dpy, ~0, &event)) {
	    XtDispatchEvent(&event);
	} else {
	    XFlush(dpy);
	    FD_ZERO(&rd);
	    FD_ZERO(&wr);
	    FD_SET(ConnectionNumber(dpy),&rd);
	    max = ConnectionNumber(dpy);
	    if (qt_hasaudio) {
		FD_SET(oss_fd,&wr);
		if (oss_fd > max)
		    max = oss_fd;
	    }
	    if (qt_hasvideo) {
		qt_frame_delay(&start,&wait);
	    } else {
		wait.tv_sec  = 1;
		wait.tv_usec = 0;
	    }
	    rc = select(max+1,&rd,&wr,NULL,&wait);
	    if (qt_hasaudio && FD_ISSET(oss_fd,&wr))
		if (0 != qt_audio_write())
		    break;
	    if (qt_hasvideo && 0 == rc)
		if (0 != qt_frame_blit())
		    break;
	}
    }
    return 0;
}


