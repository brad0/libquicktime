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
#include <X11/extensions/Xv.h>
#include <X11/extensions/Xvlib.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>

#include <quicktime/quicktime.h>
#include <quicktime/colormodels.h>

/* ------------------------------------------------------------------------ */
/* X11 code                                                                 */

static XtAppContext app_context;
static Widget       app_shell;
static Display      *dpy;
static Visual       *visual;
static XVisualInfo  vinfo,*vinfo_list;
static XPixmapFormatValues *pf;

static Widget simple;
static Dimension swidth,sheight;

static int xv_port      = 0;
static int xv_have_YUY2 = 0;
static int xv_have_I420 = 0;

static int no_mitshm    = 0;
static int pixmap_bytes = 0;
static int x11_byteswap = 0;
static int use_gl       = 0;

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
	fprintf(stderr,"can't handle visuals != TrueColor, sorry\n");
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

static void xv_init(void)
{
    int ver, rel, req, ev, err, i;
    int adaptors, formats;
    XvAdaptorInfo        *ai;
    XvImageFormatValues  *fo;
    
    if (Success != XvQueryExtension(dpy,&ver,&rel,&req,&ev,&err))
	return;

    /* find + lock port */
    if (Success != XvQueryAdaptors(dpy,DefaultRootWindow(dpy),&adaptors,&ai))
	return;
    for (i = 0; i < adaptors; i++) {
	if ((ai[i].type & XvInputMask) && (ai[i].type & XvImageMask)) {
	    if (Success != XvGrabPort(dpy,ai[i].base_id,CurrentTime)) {
		fprintf(stderr,"INFO: Xvideo port %ld: is busy, skipping\n",
			ai[i].base_id);
		continue;
	    }
	    xv_port = ai[i].base_id;
	    break;
	}
    }
    if (0 == xv_port)
	return;

    /* check image formats */
    fo = XvListImageFormats(dpy, xv_port, &formats);
    for(i = 0; i < formats; i++) {
	fprintf(stderr, "INFO: Xvideo port %d: 0x%x (%c%c%c%c) %s",
		xv_port,
		fo[i].id,
		(fo[i].id)       & 0xff,
		(fo[i].id >>  8) & 0xff,
		(fo[i].id >> 16) & 0xff,
		(fo[i].id >> 24) & 0xff,
		(fo[i].format == XvPacked) ? "packed" : "planar");
	if (FOURCC_YUV2 == fo[i].id) {
	    fprintf(stderr," [BC_YUV422]");
	    xv_have_YUY2 = 1;
	}
	if (FOURCC_YV12 == fo[i].id) {
	    fprintf(stderr," [BC_YUV420P]");
	    xv_have_I420 = 1;
	}
	fprintf(stderr,"\n");
    }
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
    if ((void *)-1 != shminfo->shmaddr  &&  NULL != shminfo->shmaddr)
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

static XvImage*
xv_create_ximage(Display *dpy, int width, int height, int port, int format)
{
    XvImage         *xvimage = NULL;
    unsigned char   *ximage_data;
    XShmSegmentInfo *shminfo = NULL;
    void            *old_handler;
    
    if (no_mitshm)
	goto no_mitshm;
    
    old_handler = XSetErrorHandler(catch_no_mitshm);
    shminfo = malloc(sizeof(XShmSegmentInfo));
    memset(shminfo, 0, sizeof(XShmSegmentInfo));
    xvimage = XvShmCreateImage(dpy, port, format, 0,
			       width, height, shminfo);
    if (NULL == xvimage)
	goto shm_error;
    shminfo->shmid = shmget(IPC_PRIVATE, xvimage->data_size,
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
    xvimage->data = shminfo->shmaddr;
    shminfo->readOnly = False;
    
    XShmAttach(dpy, shminfo);
    XSync(dpy, False);
    if (no_mitshm)
	goto shm_error;
    shmctl(shminfo->shmid, IPC_RMID, 0);
    XSetErrorHandler(old_handler);

shm_error:
    if (xvimage) {
	XFree(xvimage);
	xvimage = NULL;
    }
    if ((void *)-1 != shminfo->shmaddr  &&  NULL != shminfo->shmaddr)
	shmdt(shminfo->shmaddr);
    free(shminfo);
    XSetErrorHandler(old_handler);
    no_mitshm = 1;

 no_mitshm:
    if (NULL == (ximage_data = malloc(width * height * 2))) {
	fprintf(stderr,"out of memory\n");
	exit(1);
    }
    xvimage = XvCreateImage(dpy, port, format, ximage_data,
			    width, height);
    return xvimage;
}

static void x11_blit(Window win, GC gc, XImage *xi, int width, int height)
{
    if (no_mitshm)
	XPutImage(dpy,win,gc,xi, 0,0,0,0, width,height);
    else
	XShmPutImage(dpy,win,gc,xi, 0,0,0,0, width,height, True);
}

static void xv_blit(Window win, GC gc, XvImage *xi,
		    int iw, int ih, int ww, int wh)
{
    if (no_mitshm)
	XvPutImage(dpy,xv_port,win,gc,xi, 0,0,iw,ih, 0,0,ww,wh);
    else
	XvShmPutImage(dpy,xv_port,win,gc,xi, 0,0,iw,ih, 0,0,ww,wh, True);
}

/* ------------------------------------------------------------------------ */
/* OpenGL code                                                              */

static int tw,th;
static GLint tex;
static int gl_attrib[] = { GLX_RGBA,
			   GLX_RED_SIZE, 1,
			   GLX_GREEN_SIZE, 1,
			   GLX_BLUE_SIZE, 1,
			   GLX_DOUBLEBUFFER,
			   None };

static void gl_resize(Widget widget, int width, int height)
{
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, width, 0.0, height);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

}

static void gl_blit(Widget widget, char *rgbbuf,
		    int iw, int ih, int ww, int wh)
{
    char *dummy;
    float x,y;

    if (0 == tex) {
	glGenTextures(1,&tex);
	glBindTexture(GL_TEXTURE_2D,tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	dummy = malloc(tw*th*3);
	memset(dummy,128,tw*th*3);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,tw,th,0,
		     GL_RGB,GL_UNSIGNED_BYTE,dummy);
	free(dummy);
    }
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0,0,iw,ih,
		    GL_RGB,GL_UNSIGNED_BYTE,rgbbuf);
    x = (float)iw/tw;
    y = (float)ih/th;

    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    glBegin(GL_QUADS);
    glTexCoord2f(0,y);  glVertex3f(0,0,0);
    glTexCoord2f(0,0);  glVertex3f(0,wh,0);
    glTexCoord2f(x,0);  glVertex3f(ww,wh,0);
    glTexCoord2f(x,y);  glVertex3f(ww,0,0);
    glEnd();
    glXSwapBuffers(XtDisplay(widget), XtWindow(widget));
    glDisable(GL_TEXTURE_2D);
}

static void gl_init(Widget widget, int iw, int ih)
{
    XVisualInfo *visinfo;
    GLXContext ctx;
    int i = 0;

    visinfo = glXChooseVisual(XtDisplay(widget),
			      DefaultScreen(XtDisplay(widget)),
			      gl_attrib);
    if (!visinfo) {
	fprintf(stderr,"WARNING: gl: can't get visual (rgb,db)\n");
	return;
    }
    ctx = glXCreateContext(dpy, visinfo, NULL, True);
    glXMakeCurrent(XtDisplay(widget),XtWindow(widget),ctx);
    fprintf(stderr, "INFO: gl: DRI=%s\n",
	    glXIsDirect(dpy, ctx) ? "Yes" : "No");
    if (!glXIsDirect(dpy, ctx))
	return;
    
    /* check against max size */
    glGetIntegerv(GL_MAX_TEXTURE_SIZE,&i);
    fprintf(stderr,"INFO: gl: texture max size: %d\n",i);
    if (iw > i)
	return;
    if (ih > i)
	return;
    
    /* textures have power-of-two x,y dimensions */
    for (i = 0; iw >= (1 << i); i++)
	;
    tw = (1 << i);
    for (i = 0; ih >= (1 << i); i++)
	;
    th = (1 << i);
    fprintf(stderr,"INFO: gl: frame=%dx%d, texture=%dx%d\n",iw,ih,tw,th);

    glClearColor (0.0, 0.0, 0.0, 0.0);
    glShadeModel(GL_FLAT);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    use_gl = 1;
}

/* ------------------------------------------------------------------------ */
/* oss code                                                                 */

static int oss_fd = -1;
static int oss_sr,oss_hr;

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
	oss_sr = rate;
	oss_hr = hw_rate;
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
	fprintf(stderr,"WARNING: open %s: %s\n",dev,strerror(errno));
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
static XvImage *qt_xvimage;
static GC qt_gc;

static int16_t *qt_audio,*qt1,*qt2;
static int qt_size,qt_offset,qt_stereo;
static int qt_cmodel = BC_RGB888;

static void qt_init(FILE *fp, char *filename)
{
    char *str;
    int i;

    /* open file */
    qt = quicktime_open(filename,1,0);
    if (NULL == qt) {
	fprintf(fp,"ERROR: can't open file: %s\n",filename);
	exit(1);
    }

    /* print misc info */
    fprintf(fp,"INFO: playing %s\n",filename);
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
	qt_frame = malloc(qt_width * qt_height * 4);
	qt_rows = malloc(qt_height * sizeof(char*));
	qt_gc = XCreateGC(dpy,XtWindow(simple),0,NULL);
	switch (qt_cmodel) {
	case BC_RGB888:
	    fprintf(stderr,"INFO: using BC_RGB888 + %s\n",
		    use_gl ? "OpenGL" : "plain X11");
	    qt_ximage = x11_create_ximage(dpy,qt_width,qt_height);
	    for (i = 0; i < qt_height; i++)
		qt_rows[i] = qt_frame + qt_width * 3 * i;
	    break;
	case BC_YUV422:
	    fprintf(stderr,"INFO: using BC_YUV422 + Xvideo extention\n");
	    qt_xvimage = xv_create_ximage(dpy,qt_width,qt_height,
					  xv_port,FOURCC_YUV2);
	    for (i = 0; i < qt_height; i++)
		qt_rows[i] = qt_xvimage->data + qt_width * 2 * i;
	    break;
	case BC_YUV420P:
	    fprintf(stderr,"INFO: using BC_YUV420P + Xvideo extention\n");
	    qt_xvimage = xv_create_ximage(dpy,qt_width,qt_height,
					  xv_port,FOURCC_I420);
	    qt_rows[0] = qt_xvimage->data;
	    qt_rows[1] = qt_xvimage->data + qt_width * qt_height;
	    qt_rows[2] = qt_xvimage->data + qt_width * qt_height * 5 / 4;
	    break;
	default:
	    fprintf(stderr,"ERROR: internal error at %s:%d\n",
		    __FILE__,__LINE__);
	    exit(1);
	}
    }

    if (qt_drop) {
	qt_droptotal += qt_drop;
	fprintf(stderr,"dropped %d frame(s)\r",qt_droptotal);
	for (i = 0; i < qt_drop; i++)
	    quicktime_read_frame(qt,qt_frame,0);
	qt_drop = 0;
    }
    quicktime_decode_scaled(qt,0,0,qt_width,qt_height,qt_width,qt_height,
			    qt_cmodel,qt_rows,0);
    switch (qt_cmodel) {
    case BC_RGB888:
	if (use_gl) {
	    gl_blit(simple,qt_frame,qt_width,qt_height,swidth,sheight);
	} else {
	    switch (pixmap_bytes) {
	    case 2:
		rgb_to_lut2(qt_ximage->data,qt_frame,qt_width*qt_height);
		break;
	    case 4:
		rgb_to_lut4(qt_ximage->data,qt_frame,qt_width*qt_height);
		break;
	    }
	    x11_blit(XtWindow(simple),qt_gc,qt_ximage,qt_width,qt_height);
	}
	break;
    case BC_YUV422:
    case BC_YUV420P:
	xv_blit(XtWindow(simple),qt_gc,qt_xvimage,
		qt_width,qt_height,swidth,sheight);
	break;
    default:
	fprintf(stderr,"ERROR: internal error at %s:%d\n",
		__FILE__,__LINE__);
	exit(1);
    }

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
    if (qt_hasaudio && oss_sr && oss_hr) {
	/* cheap trick to make a/v sync ... */
	msec = (long long)msec * oss_hr / oss_sr;
    }
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
	qt_hasaudio = 0;
	break;
    case 0:
	fprintf(stderr,"write dsp: Huh? no data written?\n");
	close(oss_fd);
	oss_fd = -1;
	qt_hasaudio = 0;
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

struct ARGS {
    int  xv;
    int  gl;
} args;

XtResource args_desc[] = {
    /* name, class, type, size, offset, default_type, default_addr */
    {
	/* Integer */
	"xv",
	XtCValue, XtRInt, sizeof(int),
	XtOffset(struct ARGS*,xv),
	XtRString, "1"
    },{
	"gl",
	XtCValue, XtRInt, sizeof(int),
	XtOffset(struct ARGS*,gl),
	XtRString, "1"
    }
};
const int args_count = XtNumber(args_desc);

XrmOptionDescRec opt_desc[] = {
    { "-noxv",  "xv", XrmoptionNoArg,  "0" },
    { "-nogl",  "gl", XrmoptionNoArg,  "0" },
};
const int opt_count = (sizeof(opt_desc)/sizeof(XrmOptionDescRec));

static void usage(FILE *fp, char *prog)
{
    char *p;

    p = strrchr(prog,'/');
    if (p) prog = p+1;
    fprintf(fp,
	    "\n"
	    "Very simple quicktime movie player for X11.  Just playes\n"
	    "the movie and nothing else.  No fancy gui, no controls.\n"
	    "\n"
	    "You can quit with 'Q' and 'ESC' keys.\n"
	    "\n"
	    "usage: %s [ options ] <file>\n"
	    "options:\n"
	    "  -noxv   don't use the Xvideo extention\n"
	    "  -nogl   don't use OpenGL\n"
	    "\n",
	    prog);
}

static void quit_ac(Widget widget, XEvent *event,
		    String *params, Cardinal *num_params)
{
    exit(0);
}

static void resize_ev(Widget widget, XtPointer client_data,
		      XEvent *event, Boolean *d)
{
    switch(event->type) {
    case MapNotify:
    case ConfigureNotify:
	XtVaGetValues(widget,XtNheight,&sheight,XtNwidth,&swidth,NULL);
	fprintf(stderr,"INFO: window size is %dx%d\n",swidth,sheight);
	if (use_gl)
	    gl_resize(widget,swidth,sheight);
	break;
    }
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
				  opt_desc, opt_count,
				  &argc, argv,
				  res, NULL);
    XtGetApplicationResources(app_shell,&args,
			      args_desc,args_count,
			      NULL,0);

    /* open file */
    if (argc < 2) {
	usage(stderr,argv[0]);
	exit(1);
    }
    qt_init(stdout,argv[1]);

    /* init x11 stuff */
    dpy = XtDisplay(app_shell);
    XtAppAddActions(app_context,action_table,
		    sizeof(action_table)/sizeof(XtActionsRec));
    XtVaSetValues(app_shell, XtNtitle,argv[1],NULL);
    simple = XtVaCreateManagedWidget("playback",simpleWidgetClass,app_shell,
				     XtNwidth,  qt_width,
				     XtNheight, qt_height,
				     NULL);
    XtAddEventHandler(simple,StructureNotifyMask, True, resize_ev, NULL);
    x11_init();
    if (args.xv)
	xv_init();

    /* use Xvideo? */
    if (xv_have_YUY2 && quicktime_reads_cmodel(qt,BC_YUV422,0))
	qt_cmodel = BC_YUV422;
    if (xv_have_I420 && quicktime_reads_cmodel(qt,BC_YUV420P,0))
	qt_cmodel = BC_YUV420P;

    /* use OpenGL? */
    XtRealizeWidget(app_shell);
    if (BC_RGB888 == qt_cmodel && args.gl)
	gl_init(simple,qt_width,qt_height);

    /* enter main loop */
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
