/*
 * simple quicktime movie player, needs libquicktime
 *
 *  (c) 2002 Gerd Knorr <kraxel@bytesex.org>
 *
 */

// #define USE_GL

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/ioctl.h>
#ifdef	HAVE_SYS_SOUNDCARD_H
#include <sys/soundcard.h>
#else
#ifdef HAVE_SOUNDCARD_H
#include <soundcard.h>
#endif
#endif
#ifdef HAVE_ALSA
#include <alsa/asoundlib.h>
#endif

#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xaw/Simple.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xv.h>
#include <X11/extensions/Xvlib.h>

#ifdef HAVE_GL

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#endif

#include <quicktime/lqt.h>
#include <quicktime/colormodels.h>

/* File handle */

static quicktime_t *qt = NULL;

#define COUNT_SAMPLES

#ifdef COUNT_SAMPLES
int64_t total_samples_decoded = 0;
#endif


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
static int xv_have_YV12 = 0;

static int no_mitshm    = 0;
static int pixmap_bytes = 0;
static int x11_byteswap = 0;

#ifdef HAVE_GL
static int use_gl       = 0;
#endif

static int xpos = 0;
static int ypos = 0;
static int overhead_frames = 0;

static unsigned long   lut_red[256];
static unsigned long   lut_green[256];
static unsigned long   lut_blue[256];

/* Video stuff */

static int qt_cmodel = BC_RGB888;

static int qt_cmodels[] =
  {
    BC_RGB888, /* Always supported */
    /* Placeholders for various YUV formats, set by xv_init */
    LQT_COLORMODEL_NONE,
    LQT_COLORMODEL_NONE,
    LQT_COLORMODEL_NONE,
  };


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
    int cmodel_index;
    unsigned int ver, rel, req, ev, err;
    int i;
    unsigned int adaptors;
    int formats;
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
            xv_have_YV12 = 1;
        }
	if (FOURCC_I420 == fo[i].id) {
	    fprintf(stderr," [BC_YUV420P]");
            xv_have_I420 = 1;
	}
	fprintf(stderr,"\n");
    }

    /* Fill the cmodel array */
    cmodel_index = 1;
    if(xv_have_YUY2)
      qt_cmodels[cmodel_index++] = BC_YUV422;
    if(xv_have_YV12 || xv_have_I420)
      qt_cmodels[cmodel_index++] = BC_YUV420P;
    
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
    char   *ximage_data;
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
    return ximage;

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
    char   *ximage_data;
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
    return xvimage;

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
#ifdef HAVE_GL
static int gl_texture_width,gl_texture_height;
static GLuint gl_texture;
static int gl_attrib[] = { GLX_RGBA,
                           GLX_RED_SIZE, 8,
                           GLX_GREEN_SIZE, 8,
                           GLX_BLUE_SIZE, 8,
                           GLX_DEPTH_SIZE, 8,
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

static void gl_blit(Widget widget, uint8_t *rgbbuf,
		    int iw, int ih, int ww, int wh)
{
    char *dummy;
    float x,y;

    if (0 == gl_texture) {
	glGenTextures(1,&gl_texture);
	glBindTexture(GL_TEXTURE_2D,gl_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	dummy = malloc(gl_texture_width*gl_texture_height*3);
	memset(dummy,128,gl_texture_width*gl_texture_height*3);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,gl_texture_width,
                     gl_texture_height,0,
		     GL_RGB,GL_UNSIGNED_BYTE,dummy);
	free(dummy);
    }
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0,0,iw,ih,
		    GL_RGB,GL_UNSIGNED_BYTE,rgbbuf);
    x = (float)iw/gl_texture_width;
    y = (float)ih/gl_texture_height;

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
    if (!glXIsDirect(dpy, ctx)) {
        fprintf(stderr, "WARNING: gl: Direct rendering missing\n");
        return;
    }
#if 0
    /* check against max size */
    glGetIntegerv(GL_MAX_TEXTURE_SIZE,&i);
    fprintf(stderr,"INFO: gl: texture max size: %d\n",i);
    if ((iw > i) || (ih > i))
        {
        fprintf(stderr, "WARNING: gl: Maximum texture size too small (got %dx%d, needed %dx%d)\n",
                i, i, iw, ih);
        return;
        }
#endif
    /* textures have power-of-two x,y dimensions */
    for (i = 0; iw >= (1 << i); i++)
	;
    gl_texture_width = (1 << i);
    for (i = 0; ih >= (1 << i); i++)
	;
    gl_texture_height = (1 << i);
    fprintf(stderr,"INFO: gl: frame=%dx%d, texture=%dx%d\n",iw,ih,
            gl_texture_width,gl_texture_height);

    glClearColor (0.0, 0.0, 0.0, 0.0);
    glShadeModel(GL_FLAT);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    use_gl = 1;
}

#endif // HAVE_GL

static int oss_sr,oss_hr;



/* ------------------------------------------------------------------------ */
/* alsa code                                                                 */

#ifdef HAVE_ALSA

/* Enable alsa */
static int use_alsa = 1;

#ifndef SND_PCM_FORMAT_S16_NE
# ifdef WORDS_BIGENDIAN
#  define SND_PCM_FORMAT_S16_NE SND_PCM_FORMAT_S16_BE
# else
#  define SND_PCM_FORMAT_S16_NE SND_PCM_FORMAT_S16_LE
# endif
#endif

//static int periods = 32; /* number of periods == fragments */
static snd_pcm_uframes_t periodsize = 1024; /* Periodsize (bytes) */
static unsigned int buffer_time = 500000; 
static unsigned int period_time = 125000;              /* period time in us */

/* Handle for the PCM device */ 
static snd_pcm_t *pcm_handle;

/* This structure contains information about    */
/* the hardware and can be used to specify the  */
/* configuration to be used for the PCM stream. */
static snd_pcm_hw_params_t *hwparams;
snd_pcm_uframes_t buffer_size;

#else

/* Disable Alsa */
static int use_alsa = 0;

#endif /* HAVE_ALSA */

static int alsa_init(char *dev, int channels, int rate)
{
#ifdef HAVE_ALSA
    int dir;
    //    int exact_param;   /* parameter returned by          */
                       /* snd_pcm_hw_params_set_*_near   */ 
    unsigned int tmprate;
    int err = 0;
    tmprate = rate;
    oss_hr = rate;
    if (snd_pcm_open(&pcm_handle, dev, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK) < 0) {
	fprintf(stderr, "Error opening PCM device %s\n", dev);
	return 1;
    }
    
    /* Allocate the snd_pcm_hw_params_t structure on the stack. */
    snd_pcm_hw_params_alloca(&hwparams);

    if ((err = snd_pcm_hw_params_any(pcm_handle, hwparams)) < 0) {
	fprintf(stderr, "Can not configure this PCM device. (%s)\n", snd_strerror(err));
	return 1;
    }
    
    if ((err = snd_pcm_hw_params_set_access(pcm_handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
	fprintf(stderr, "Error setting access. (%s)\n", snd_strerror(err));
	return 1;
    }

    /* put checks here */
    /* Only needed when sampling format not supported by hardware .. unlikely */

    /* Set sample format */
    if ((err = snd_pcm_hw_params_set_format(pcm_handle, hwparams,  SND_PCM_FORMAT_S16_NE)) < 0) {
	fprintf(stderr, "Error setting format.(%s)\n", snd_strerror(err));
	return 1;
    }
    
    /* Set number of channels */ // weird 1 channel mode doesn't work
    if ((err = snd_pcm_hw_params_set_channels(pcm_handle, hwparams, channels)) < 0) {
	fprintf(stderr, "Error setting channels. %i (%s)\n", channels, snd_strerror(err));
	return 1;
    }
    
    /* Set sample rate. If the exact rate is not supported */
    /* by the hardware, use nearest possible rate.         */ 
    if ((err = snd_pcm_hw_params_set_rate_near(pcm_handle, hwparams, &tmprate, 0)) < 0 ) {
	fprintf(stderr, "Error setting sample rate (%s)\n", snd_strerror(err));
	return 1;
    }

    if (tmprate != rate) fprintf(stderr,"WARNING: Using %i Hz instead of requested rate %i Hz\n ", tmprate, rate);
    oss_sr = tmprate;
    dir = 0;
    if ((err = snd_pcm_hw_params_set_buffer_time_near(pcm_handle, hwparams, &buffer_time, &dir)) < 0) {
	printf("Unable to set buffer time %i for playback: %s\n", buffer_time, snd_strerror(err));
	return 1;
    }

    /* period time */
    dir = 0;
    if ((err = snd_pcm_hw_params_set_period_time_near(pcm_handle, hwparams, &period_time, &dir)) < 0) {
	fprintf(stderr, "Error setting periods.(%s)\n", snd_strerror(err));
	return 1;
    }

    
    if ((err = snd_pcm_hw_params_get_buffer_size(hwparams, &buffer_size))< 0) {
                fprintf(stderr, "Unable to get buffer size for playback: %s\n", snd_strerror(err));
                return 1;
    }
    
    dir = 0;
    err = snd_pcm_hw_params_get_period_size(hwparams, &periodsize, &dir);
    if (err < 0) {
                fprintf(stderr, "Unable to get period size for playback: %s\n", snd_strerror(err));
                return 1;
    }

    /* Apply HW parameter settings to */
    /* PCM device and prepare device  */
    if ((err = snd_pcm_hw_params(pcm_handle, hwparams)) < 0) {
	fprintf(stderr, "Error setting HW params.(%s)\n", snd_strerror(err));
	return 1;
    }
    
    if ((err = snd_pcm_prepare(pcm_handle)) < 0) {
	fprintf(stderr, "Error in pcm_prepare.(%s)\n", snd_strerror(err));
	return 1;
    }
    
#endif

return 0;
}



/* ------------------------------------------------------------------------ */
/* oss code                                                                 */

#ifndef AFMT_S16_NE
# if BYTE_ORDER == BIG_ENDIAN
#  define AFMT_S16_NE AFMT_S16_BE
# else
#  define AFMT_S16_NE AFMT_S16_LE
# endif
#endif

static int oss_fd = -1;
static int oss_sr,oss_hr;

static int
oss_setformat(int chan, int rate)
{
#ifdef	SNDCTL_DSP_SETFMT
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
#else
    return 1;
#endif
}

static int oss_init(char *dev, int channels, int rate)
{
#ifdef	SNDCTL_DSP_SETTRIGGER
    int trigger;
#endif

    oss_fd = open(dev,O_WRONLY | O_NONBLOCK);
    if (-1 == oss_fd) {
	fprintf(stderr,"WARNING: open %s: %s\n",dev,strerror(errno));
	return -1;
    }
    oss_setformat(channels,rate);
#ifdef	SNDCTL_DSP_SETTRIGGER
    trigger = PCM_ENABLE_OUTPUT;
    ioctl(oss_fd,SNDCTL_DSP_SETTRIGGER,&trigger);
    return 0;
#else
    return 1;
#endif
}

/* ------------------------------------------------------------------------ */
/* quicktime code                                                           */

/* Audio stuff */

/* Interleaved audio buffer */
static int16_t *qt_audio = (int16_t*)0;    //,*qt1,*qt2;
static int16_t *qt_audio_ptr; /* Pointer to the sample buffer for the next write() call */

static int qt_audio_samples_in_buffer;

/* Non interleaved audio buffer */
static int16_t **qt_audion = (int16_t**)0;

/* One decode call will decode this many samples */

#define AUDIO_BLOCK_SIZE (10*1024)

static int qt_channels,qt_sample_rate;
static int qt_audio_eof = 0; /* No more samples can be decoded */

static int qt_hasvideo,qt_hasaudio,qt_isqtvr;

static int qt_width = 320, qt_height = 32, qt_drop = 0, qt_droptotal = 0, qtvr_dwidth = 0, qtvr_dheight = 0;
static int64_t qt_frame_time; /* Timestamp of the decoded frame */
static int qt_timescale = 0;

static unsigned char *qt_frame,**qt_rows;
static unsigned char *qt_panorama_buffer;
static XImage *qt_ximage;
static XvImage *qt_xvimage;
static GC qt_gc;


static void qt_init(FILE *fp, char *filename)
{
    char *str;
    int i, j;
    int ipos;
    float minpan, maxpan;
    const lqt_channel_t * channel_setup;
    int num_channels;
    /* audio device */
    char *adev_name;
    
    /* default */
    adev_name = strdup("plughw");

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
	fprintf(fp,"  name:      %s\n",str);
    str = quicktime_get_info(qt);
    if (str)
	fprintf(fp,"  info:      %s\n",str);

    str = lqt_get_author(qt);
    if (str)
	fprintf(fp,"  author:    %s\n",str);

    str = lqt_get_artist(qt);
    if (str)
	fprintf(fp,"  artist:    %s\n",str);

    str = lqt_get_album(qt);
    if (str)
        fprintf(fp,"  album:     %s\n",str);

    str = lqt_get_genre(qt);
    if (str)
	fprintf(fp,"  genre:     %s\n",str);

    str = lqt_get_track(qt);
    if (str)
	fprintf(fp,"  track:     %s\n",str);

    str = lqt_get_comment(qt);
    if (str)
	fprintf(fp,"  comment:   %s\n",str);
    
    
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
    
    qt_isqtvr = lqt_is_qtvr(qt);
    if (qt_isqtvr) {
	if (qt_isqtvr != QTVR_OBJ && qt_isqtvr != QTVR_PAN) {
	    fprintf(stderr, "'%s' is no QTVR file or an unsupported version.\n"
			    "Currently only QTVR V1.0 object and panorama files are supported.\n", filename);
	    exit(1);
	}
	
	ipos = lqt_qtvr_initial_position(qt);
	lqt_qtvr_get_pan(qt, &(minpan), &(maxpan), NULL);
	qtvr_dwidth = lqt_qtvr_get_display_width(qt);
	qtvr_dheight = lqt_qtvr_get_display_height(qt);
	fprintf(stderr, "startpos :%i\n", ipos);
	//fprintf(stderr, "fov :%f\n", lqt_qtvr_get_fov(qt));
	fprintf(stderr, "movietype :%i\n", lqt_qtvr_get_movietype(qt));
	fprintf(stderr, "panning :%f %f\n", minpan, maxpan);
	fprintf(stderr, "rows :%i\n", lqt_qtvr_get_rows(qt));
	fprintf(stderr, "colums :%i\n", lqt_qtvr_get_columns(qt));
	fprintf(stderr, "disp width :%i\n", qtvr_dwidth);
	fprintf(stderr, "disp height :%i\n", qtvr_dheight);
	fprintf(stderr, "width :%i\n", lqt_qtvr_get_width(qt));
	fprintf(stderr, "height :%i\n", lqt_qtvr_get_height(qt));
	fprintf(stderr, "depth :%i\n", lqt_qtvr_get_depth(qt));
    
	if (qt_isqtvr == QTVR_PAN) {
	    overhead_frames = (float)lqt_qtvr_get_height(qt) * ((float)qtvr_dwidth / (float)qtvr_dheight);
	    overhead_frames = floor((float)overhead_frames / (float)quicktime_video_height(qt, 0)) * (float)quicktime_video_height(qt, 0) + quicktime_video_height(qt, 0);
	    fprintf(stderr, "overhead_frames :%i\n",overhead_frames);
	}
    }
    /* print audio info */
    if (quicktime_has_audio(qt)) {
	fprintf(fp,"  audio: %d track(s)\n",quicktime_audio_tracks(qt));
	for (i = 0; i < quicktime_audio_tracks(qt); i++) {
          {
          num_channels = quicktime_track_channels(qt,i);
          fprintf(fp,
                  "    track #%d\n"
                  "      rate  : %ld Hz\n"
                  "      bits  : %d\n"
                  "      chans : %d\n"
                  "      codec : %s\n",
                  i+1,
                  quicktime_sample_rate(qt,i),
                  quicktime_audio_bits(qt,i),
                  num_channels,
                  quicktime_audio_compressor(qt,i));
          channel_setup = lqt_get_channel_setup(qt, i);
          fprintf(fp, "    channel_setup : ");
          if(channel_setup)
            {
            for(j = 0; j < num_channels; j++)
              {
              fprintf(fp, "%s", lqt_channel_to_string(channel_setup[j]));
              if(j < num_channels - 1)
                fprintf(fp, ", ");
              }
            fprintf(fp, "\n");
            }
          else
            fprintf(fp, "Not available\n");
          }
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
        qt_timescale = lqt_video_time_scale(qt,0);
        fprintf(stderr, "Timescale: %d\n", qt_timescale);
    }

    if (!quicktime_has_audio(qt)) {
	fprintf(stderr,"WARNING: no audio stream\n");
    } else if (!quicktime_supported_audio(qt,0)) {
	fprintf(stderr,"WARNING: unsupported audio codec\n");
    } else {
	qt_hasaudio = 1;
	qt_channels = quicktime_track_channels(qt,0);
        qt_sample_rate = quicktime_sample_rate(qt,0);
  if (use_alsa == 1) {
    if (-1 == alsa_init(adev_name, qt_channels, 
         qt_sample_rate)) {
           qt_hasaudio = 0;}
  }
  else {
  	if (-1 == oss_init("/dev/dsp", qt_channels,
                           qt_sample_rate))
           qt_hasaudio = 0;
  }
  }
    if (0 == qt_hasvideo && 0 == qt_hasaudio) {
	fprintf(stderr,"ERROR: no playable stream found\n");
	exit(1);
    }
}

static void qt_cleanup()
  {
  if(qt)
    quicktime_close(qt);
  }

static int qt_init_video(void)
{
        int i;
        /* init */
	
	qt_rows = malloc(qt_height * sizeof(char*));
	if (qt_isqtvr == QTVR_PAN) {
	    qt_frame = malloc(qtvr_dwidth * qtvr_dheight * 4);
	    qt_panorama_buffer = malloc((lqt_qtvr_get_width(qt)+overhead_frames)*
					 lqt_qtvr_get_height(qt)*
					 3);
	}
	else
	    qt_frame = malloc(qt_width * qt_height * 4);


	qt_gc = XCreateGC(dpy,XtWindow(simple),0,NULL);
	switch (qt_cmodel) {
	case BC_RGB888:
#ifdef HAVE_GL
          fprintf(stderr,"INFO: using BC_RGB888 + %s\n",
		    use_gl ? "OpenGL" : "plain X11");
#else
          fprintf(stderr,"INFO: using BC_RGB888 + %s\n",
		    "plain X11");
#endif
	    if (qt_isqtvr == QTVR_PAN) {
		qt_ximage = x11_create_ximage(dpy,qtvr_dwidth,qtvr_dheight);
	    }
	    else
		qt_ximage = x11_create_ximage(dpy,qt_width,qt_height);
	    
	    for (i = 0; i < qt_height; i++)
		qt_rows[i] = qt_frame + qt_width * 3 * i;
	    break;
	case BC_YUV422:
	    fprintf(stderr,"INFO: using BC_YUV422 + Xvideo extention\n");
	    qt_xvimage = xv_create_ximage(dpy,qt_width,qt_height,
					  xv_port,FOURCC_YUV2);
	    for (i = 0; i < qt_height; i++)
              qt_rows[i] = (uint8_t*)(qt_xvimage->data + qt_width * 2 * i);

            lqt_set_row_span(qt,0,qt_xvimage->pitches[0]);
            
            break;
        case BC_YUV420P:
            if(xv_have_YV12)
              {
              fprintf(stderr,
                      "INFO: using BC_YUV420P + Xvideo extention (YV12)\n");
              qt_xvimage = xv_create_ximage(dpy,qt_width,qt_height,
              xv_port,FOURCC_YV12);
              qt_rows[0] = (uint8_t*)(qt_xvimage->data + qt_xvimage->offsets[0]);
              qt_rows[1] = (uint8_t*)(qt_xvimage->data + qt_xvimage->offsets[2]);
              qt_rows[2] = (uint8_t*)(qt_xvimage->data + qt_xvimage->offsets[1]);
              }
	    else if(xv_have_I420)
              {
              fprintf(stderr,
                      "INFO: using BC_YUV420P + Xvideo extention (I420)\n");
              qt_xvimage = xv_create_ximage(dpy,qt_width,qt_height,
	                                    xv_port,FOURCC_I420);
	      qt_rows[0] = (uint8_t*)(qt_xvimage->data + qt_xvimage->offsets[0]);
	      qt_rows[1] = (uint8_t*)(qt_xvimage->data + qt_xvimage->offsets[1]);
	      qt_rows[2] = (uint8_t*)(qt_xvimage->data + qt_xvimage->offsets[2]);
	      }
            lqt_set_row_span(qt,0,qt_xvimage->pitches[0]);
            lqt_set_row_span_uv(qt,0,qt_xvimage->pitches[1]);
            break;
	default:
	    fprintf(stderr,"ERROR: internal error at %s:%d\n",
		    __FILE__,__LINE__);
	    exit(1);
	}
	if (qt_isqtvr) {
	    /* has to be done here to set initial pov */ // look into this
	    quicktime_set_video_position(qt, lqt_qtvr_initial_position(qt),0);
	    /*XSetInputFocus(dpy, XtWindow(simple), RevertToPointerRoot, CurrentTime);
	    */
	    if (qt_isqtvr == QTVR_PAN) {
		int i,l,j,k;
		long buffer_size;
      
		buffer_size = (lqt_qtvr_get_width(qt) + overhead_frames) * lqt_qtvr_get_height(qt) * 3;
		for ( l = 0; l < lqt_qtvr_get_columns(qt) + (int)(overhead_frames/qt_height); l++) {
		    /* set position in video file */
		    /* if position larger than panorama, start from the beginning (overhead)*/
		    quicktime_set_video_position(qt,
						 (l>=lqt_qtvr_get_columns(qt))?l-lqt_qtvr_get_columns(qt):l,
						 0);
		    /* decode a frame */
		    lqt_decode_video(qt, qt_rows, 0);
		    
		    /* set position in the output buffer */
		    /* to the upper left corner of the desired frame */
		    j = l * qt_height * 3;
		    
		    /* put frame, rotated into buffer */
		    for (k = 0; k < qt_width; k++) {
			for (i = qt_height - 1; i >= 0; i--) {
			    qt_panorama_buffer[buffer_size - j + 0] = qt_frame[((qt_height - i) * qt_width - k) * 3 + 0];
			    qt_panorama_buffer[buffer_size - j + 1] = qt_frame[((qt_height - i) * qt_width - k) * 3 + 1];
			    qt_panorama_buffer[buffer_size - j + 2] = qt_frame[((qt_height - i) * qt_width - k) * 3 + 2];
			    j = j + 3;
			}
			j += (lqt_qtvr_get_width(qt) + overhead_frames - qt_height) * 3;
		    }
		}
	    }
	}
        return 0;
}

static int qt_frame_decode(void)
  {
  int i;
  
  if (qt_isqtvr == QTVR_OBJ) {
	/* sanity check and decode  */
	if (quicktime_video_position(qt, 0) >= 0 &&
	    quicktime_video_position(qt, 0) < quicktime_video_length(qt,0))
	    lqt_decode_video(qt, qt_rows, 0);
	//    quicktime_decode_scaled(qt,0,0,qt_width,qt_height,qt_width,qt_height,
    //			    qt_cmodel,qt_rows,0);
    }
    else if (qt_isqtvr == QTVR_PAN) {
	int i, j, k = 0;
	i = (ypos * (lqt_qtvr_get_width(qt) + overhead_frames) + xpos) * 3;
	do {
	    for (j = 0; j < qtvr_dwidth * 3; j++) {
		qt_frame[k] = qt_panorama_buffer[i];
		i++; k++;
	    }
	    i+= (lqt_qtvr_get_width(qt) + overhead_frames - qtvr_dwidth) * 3;
	} while (k < (qtvr_dwidth * qtvr_dheight) * 3);

	do {
	    for (j = 0; j < qtvr_dwidth * 3; j++) {
		qt_frame[k] = qt_panorama_buffer[i];
		i++; k++;
	    }
	    i+= (lqt_qtvr_get_width(qt) + overhead_frames - qtvr_dwidth) * 3;
	} while (k < (qtvr_dwidth * qtvr_dheight) * 3);
    }	
    else {
	if (quicktime_video_position(qt,0) >= quicktime_video_length(qt,0))
	    return -1;
    
	if (qt_drop) {
	    	qt_droptotal += qt_drop;
		fprintf(stderr,"dropped %d frame(s)\r",qt_droptotal);
	    for (i = 0; i < qt_drop; i++)
		quicktime_read_frame(qt,qt_frame,0);
	    qt_drop = 0;
	  }
	qt_frame_time = lqt_frame_time(qt, 0);
//        fprintf(stderr, "Frame time: %lld\n", qt_frame_time);
	lqt_decode_video(qt, qt_rows, 0);
//        quicktime_decode_scaled(qt,0,0,qt_width,qt_height,qt_width,qt_height,
//			    qt_cmodel,qt_rows,0);

    }
  return 0;
  }

static int qt_frame_blit(void)
{

    switch (qt_cmodel) {
    case BC_RGB888:
#ifdef HAVE_GL
      if (use_gl) {
	  if (qt_isqtvr == QTVR_PAN) {
	    gl_blit(simple,qt_frame,qtvr_dwidth,qtvr_dheight,swidth,sheight);
	  } else gl_blit(simple,qt_frame,qt_width,qt_height,swidth,sheight);
	} else
#endif // HAVE_GL

        {
	    switch (pixmap_bytes) {
	    case 2:
                if (qt_isqtvr == QTVR_PAN) {
		    rgb_to_lut2((uint8_t*)(qt_ximage->data),qt_frame,qtvr_dwidth*qtvr_dheight);
		} else rgb_to_lut2((uint8_t*)(qt_ximage->data),qt_frame,qt_width*qt_height);
		break;
	    case 4:
            if (qt_isqtvr == QTVR_PAN) {
		rgb_to_lut4((uint8_t*)(qt_ximage->data),qt_frame,qtvr_dwidth*qtvr_dheight);
	    } else  rgb_to_lut4((uint8_t*)(qt_ximage->data),qt_frame,qt_width*qt_height);
		break;
	    }
	    if (qt_isqtvr == QTVR_PAN) {
		x11_blit(XtWindow(simple),qt_gc,qt_ximage,qtvr_dwidth,qtvr_dheight);
	    } else
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

    return 0;
}

static void qt_frame_delay(struct timeval *start, struct timeval *wait)
{
    struct timeval now;
    long msec;

    gettimeofday(&now,NULL);
    /* Get the now - start (->negative value) */
    msec  = (start->tv_sec  - now.tv_sec)  * 1000;
    msec += (start->tv_usec - now.tv_usec) / 1000;
    if (qt_hasaudio && oss_sr && oss_hr) {
	/* cheap trick to make a/v sync ... */
	msec = (long long)msec * oss_hr / oss_sr;
    }
    
    msec += (qt_frame_time * 1000) / qt_timescale;

    //    fprintf(stderr, "Time (msec) %ld\n", (qt_frame_time * 1000) / qt_timescale);

    if (msec < 0) {
	qt_drop = -msec * quicktime_frame_rate(qt,0) / 1000;
	wait->tv_sec  = 0;
	wait->tv_usec = 0;
    } else {
	wait->tv_sec  = msec / 1000;
	wait->tv_usec = (msec % 1000) * 1000;
    }
}


/* Decode at most AUDIO_BLOCK_SIZE samples and interleave them into the
   qt_audio array. Return the real number of decoded samples */

static int decode_audio()
  {
  int i, j;
  int samples_decoded;
  long last_pos = lqt_last_audio_position(qt, 0);
  if(!qt_audio)
    {
    /* Initialize */

    if(qt_channels > 1)
      {
      qt_audion  = malloc(qt_channels * sizeof(*qt_audion));
      for(i = 0; i < qt_channels; i++)
        {
        qt_audion[i] = calloc(AUDIO_BLOCK_SIZE, sizeof(*(qt_audion[i])));
        }
      }
    qt_audio  = calloc(AUDIO_BLOCK_SIZE*qt_channels, sizeof(*qt_audio));
    }
  
  if(qt_channels > 1)
    {
    lqt_decode_audio_track(qt, qt_audion, (float**)0, AUDIO_BLOCK_SIZE, 0);
    samples_decoded = lqt_last_audio_position(qt, 0) - last_pos;
    
    /* Interleave */
    for (i = 0; i < samples_decoded; i++)
      {
      for(j = 0; j < qt_channels; j++)
        {
        qt_audio[qt_channels*i+j] = qt_audion[j][i];
        }
      }
    }
  else
    {
    //    fprintf(stderr, "Decode audio...");
    lqt_decode_audio_track(qt, &qt_audio, (float**)0, AUDIO_BLOCK_SIZE, 0);
    //    fprintf(stderr, "done\n");
    samples_decoded = lqt_last_audio_position(qt, 0) - last_pos;
    }
  total_samples_decoded += samples_decoded;
  qt_audio_samples_in_buffer = samples_decoded;
  qt_audio_ptr = qt_audio;
  if(samples_decoded < AUDIO_BLOCK_SIZE)
    {
    fprintf(stderr, "Audio track finished (got %d samples, wanted %d)\n",
            samples_decoded, AUDIO_BLOCK_SIZE);
    qt_audio_eof = 1;
    }
  return samples_decoded;
  }

//static int runcount;
static int qt_alsa_audio_write()
{
#ifdef HAVE_ALSA
    int done = 0;
    int ret = 0;
    //    fprintf(stderr, "Write alsa %d\n", qt_audio_samples_in_buffer);
    while(!done) {
        /* Decode new audio samples */
        if(!qt_audio_samples_in_buffer)
          decode_audio(AUDIO_BLOCK_SIZE);
        //        fprintf(stderr, "snd_pcm_writei..");
        ret = snd_pcm_writei(pcm_handle, (void *)(qt_audio_ptr), qt_audio_samples_in_buffer);
        //        fprintf(stderr, "done %d\n", ret);
        if (ret == -EAGAIN) { 
            ret = 0;
            done = 1;
    //           snd_pcm_wait(pcm_handle, 1000);
        }
        else if (ret == -EPIPE) {
            snd_pcm_prepare(pcm_handle);
            fprintf(stderr, "Warning: buffer underrun\n");
        }
        else if (ret < 0) {
            fprintf(stderr, "Warning: %s\n", snd_strerror(ret));
        }
        else if (ret >= 0)
          done = 1;
    }
    if (ret > 0 ) {
    qt_audio_samples_in_buffer -=ret;
    qt_audio_ptr += ret * qt_channels;
    }
    
    if (qt_audio_eof && 0 == qt_audio_samples_in_buffer) {
        snd_pcm_drain(pcm_handle);
        return -1;
    }
#endif

  return 0;
}

static int qt_oss_audio_write(void)
{
    int rc;

    if(!qt_audio_samples_in_buffer)
      decode_audio(AUDIO_BLOCK_SIZE);
    
    rc = write(oss_fd,qt_audio_ptr,qt_audio_samples_in_buffer * qt_channels * sizeof(*qt_audio));
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
        qt_audio_samples_in_buffer -= rc / (qt_channels * sizeof(*qt_audio));
        qt_audio_ptr += rc / sizeof(*qt_audio);
        break;
    }

    if (qt_audio_eof && 0 == qt_audio_samples_in_buffer) {
       return -1;
    }
    return 0;
}

/* ------------------------------------------------------------------------ */
/* main                                                                     */

struct ARGS {
    int  xv;
    int  gl;
    int  alsa;
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
    },{
	"alsa",
	XtCValue, XtRInt, sizeof(int),
	XtOffset(struct ARGS*,alsa),
	XtRString, "1"
    }
};
const int args_count = XtNumber(args_desc);

XrmOptionDescRec opt_desc[] = {
    { "-noxv",  "xv", XrmoptionNoArg,  "0" },
    { "-nogl",  "gl", XrmoptionNoArg,  "0" },
    { "-noalsa",  "alsa", XrmoptionNoArg,  "0" },
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
	    "  -noalsa don't use Alsa\n"
	    "\n",
	    prog);
}

static void decode_qtvr()
{
    XEvent event;
    int startpos;

    startpos = quicktime_video_position(qt, 0);

    XFlush(dpy);
    if (qt_hasvideo) {
	if (0 != qt_frame_decode()) exit(0);
	if (0 != qt_frame_blit()) exit(0);
    }

    for (;;) {
	struct timespec time;
	
//      printf("loop pos :%ld\n",quicktime_video_position(qt,0));
	if (quicktime_video_position(qt,0) >= startpos + lqt_qtvr_get_loop_frames(qt))
	    quicktime_set_video_position(qt, startpos, 0);

	if (True == XCheckMaskEvent(dpy, ~0, &event)) {
	    XtDispatchEvent(&event);
	} else {
//	printf("loop pos before decode :%ld\n",quicktime_video_position(qt,0));
	    XFlush(dpy);
	    XtAppProcessEvent(app_context, KeyPressMask||KeyReleaseMask);
	    time.tv_nsec = 10000000;
	    time.tv_sec = 0;
	    nanosleep(&time, NULL);
	}
    }
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
#ifdef HAVE_GL
	if (use_gl)
	    gl_resize(widget,swidth,sheight);
#endif
	if (qt_isqtvr != 0) decode_qtvr();
	break;
    case UnmapNotify:
    case DestroyNotify:
    	exit(0);
	break;
    }
}

/* event handlers */

static void left_ac(Widget widget, XEvent *event,
		    String *params, Cardinal *num_params)
{
    if (qt_isqtvr == QTVR_PAN) {
	if (xpos - 10 >= 0 ) xpos -= 10;
	if (xpos - 10 < 0) {
	    xpos = xpos-10 + lqt_qtvr_get_width(qt);
	}
    }
    else if (qt_isqtvr == QTVR_OBJ) {
	int vpos = quicktime_video_position(qt, 0);
	int lfxcolumns = lqt_qtvr_get_loop_frames(qt) * lqt_qtvr_get_columns(qt);
	
	if (vpos % lfxcolumns == lfxcolumns - lqt_qtvr_get_loop_frames(qt) && vpos != 0) {
	    quicktime_set_video_position(qt, vpos - (lqt_qtvr_get_columns(qt)-1), 0); 
	} 
	else {	
	    quicktime_set_video_position(qt, vpos + lqt_qtvr_get_loop_frames(qt), 0);
	}
    }
    else
	return;
//    printf("pos %ld\n",quicktime_video_position(qt,0));
    decode_qtvr();
}


static void right_ac(Widget widget, XEvent *event,
		    String *params, Cardinal *num_params)
{
    if (qt_isqtvr == QTVR_PAN) {
	if (xpos + 10 <= lqt_qtvr_get_width(qt)+overhead_frames - qtvr_dwidth) xpos += 10;
	if (xpos + 10 > lqt_qtvr_get_width(qt)) {
	    xpos = xpos+10 - lqt_qtvr_get_width(qt);
	}
    }
    else if (qt_isqtvr == QTVR_OBJ) {
	int vpos = quicktime_video_position(qt, 0);
	int lfxcolumns = lqt_qtvr_get_loop_frames(qt) * lqt_qtvr_get_columns(qt);
    
	if (vpos % lfxcolumns == 0) { 
	    quicktime_set_video_position(qt, vpos + (lqt_qtvr_get_loop_frames(qt) * lqt_qtvr_get_columns(qt)-1), 0);
	} 
	else {
	    quicktime_set_video_position(qt, vpos - lqt_qtvr_get_loop_frames(qt), 0);
	}
    }
    else
	return;
//    printf("pos %ld\n",quicktime_video_position(qt,0));
    decode_qtvr();
}


static void up_ac(Widget widget, XEvent *event,
		    String *params, Cardinal *num_params)
{
    if (qt_isqtvr == QTVR_PAN) {
	if (ypos - 10 >= 0 ) ypos -= 10;
    }
    else if (qt_isqtvr == QTVR_OBJ) {
	int vpos = quicktime_video_position(qt, 0);

	if (vpos + lqt_qtvr_get_columns(qt) < lqt_qtvr_get_loop_frames(qt) * lqt_qtvr_get_rows(qt) * lqt_qtvr_get_columns(qt)) {
	    quicktime_set_video_position(qt ,vpos + lqt_qtvr_get_columns(qt), 0);
	}
//  printf("pos %ld\n",quicktime_video_position(qt,0));
    }
    else 
	return;
    decode_qtvr();
}


static void down_ac(Widget widget, XEvent *event,
		    String *params, Cardinal *num_params)
{
    if (qt_isqtvr == QTVR_PAN) {
	if (ypos + 10 <= lqt_qtvr_get_height(qt) - lqt_qtvr_get_display_height(qt)) ypos += 10;
    }
    else if (qt_isqtvr == QTVR_OBJ) {
	if (quicktime_video_position(qt, 0) - lqt_qtvr_get_columns(qt) >= 0) {
	    quicktime_set_video_position(qt ,quicktime_video_position(qt, 0) - lqt_qtvr_get_columns(qt), 0);
	}
    }
    else 
	return;
//  printf("pos %ld\n",quicktime_video_position(qt,0));    
    decode_qtvr();
}

static void expose_ev(Widget widget, XtPointer client_data,
		      XEvent *event, Boolean *d)
{
    if (event->type == Expose)
	decode_qtvr();
}

static XtActionsRec action_table[] = {
    { "Quit",  quit_ac },
    { "Right", right_ac },
    { "Left",  left_ac },
    { "Up",    up_ac },
    { "Down",  down_ac },
};

static String res[] = {
    "lqtplay.playback.translations:  #override \\n"
    "        <Key>Q:                 Quit()    \\n"
    "        <Key>Escape:            Quit()    \\n"
    "        <Key>Right:             Right()    \\n"
    "        <Key>Left:              Left()   \\n"
    "        <Key>Up:                Up() \\n"
    "        <Key>Down:              Down()",
    "lqtplay.playback.background:    black",
    NULL
};

int main(int argc, char *argv[])
{
    int has_frame = 0, blit_frame = 0;
    struct timeval start,wait;
    //    int audio_frames;
    
    app_shell = XtVaAppInitialize(&app_context, "lqtplay",
				  opt_desc, opt_count,
				  &argc, argv,
				  res, NULL);
    XtGetApplicationResources(app_shell,&args,
			      args_desc,args_count,
			      NULL,0);

    /* don't use alsa*/			      
    if (!args.alsa) use_alsa=0;
			      
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
    if (qt_isqtvr) {
	simple = XtVaCreateManagedWidget("playback",simpleWidgetClass,app_shell,
			     		 XtNwidth,  qtvr_dwidth,
					 XtNheight, qtvr_dheight,
					 NULL);
	XtAddEventHandler(simple,ExposureMask, True, expose_ev, NULL);
    }
    else
	simple = XtVaCreateManagedWidget("playback",simpleWidgetClass,app_shell,
			     		 XtNwidth,  qt_width,
					 XtNheight, qt_height,
					 NULL);


    XtAddEventHandler(simple,StructureNotifyMask, True, resize_ev, NULL);

    x11_init();
    if (args.xv && qt_hasvideo)
	xv_init();
    if(qt_hasvideo) {
        if (qt_isqtvr == QTVR_PAN) {
	    /* cmodel hardcoded for panos */
	    lqt_set_cmodel(qt, 0, BC_RGB888);
	}
	else {
	    /* Decide about the colormodel */
	    fprintf(stderr, "Stream colormodel %s, ", lqt_colormodel_to_string(lqt_get_cmodel(qt, 0)));
	    qt_cmodel = lqt_get_best_colormodel(qt, 0, qt_cmodels);
	    fprintf(stderr, "using %s\n", lqt_colormodel_to_string(qt_cmodel));
	    /* Set decoding colormodel */
	    lqt_set_cmodel(qt, 0, qt_cmodel);
	}
    }
    /* use OpenGL? */
    XtRealizeWidget(app_shell);
#ifdef HAVE_GL
    if (BC_RGB888 == qt_cmodel && args.gl && qt_hasvideo) {
    	if (qt_isqtvr == QTVR_PAN) {
	    gl_init(simple,qtvr_dwidth,qtvr_dheight);
	} else 
	    gl_init(simple,qt_width,qt_height);
    }
#endif    
    /* frames per chunk for alsa */
#if 0
    if(qt_hasvideo)
      audio_frames = ((oss_sr / quicktime_frame_rate(qt,0)) / 2 + 0.5);
    else
      audio_frames = oss_sr + 1024;
#endif
    /* Initialize video */
    if(qt_hasvideo)
	qt_init_video();

    if (qt_isqtvr) {
	decode_qtvr();
    }
    else {
	/* enter main loop */
	gettimeofday(&start,NULL);
	for (;;) {
	    int rc,max;
	    fd_set rd,wr;
	    XEvent event;

	    if (True == XCheckMaskEvent(dpy, ~0, &event)) {
		XtDispatchEvent(&event);
	    } 
	    else {
		XFlush(dpy);
		FD_ZERO(&rd);
		FD_ZERO(&wr);
		FD_SET(ConnectionNumber(dpy),&rd);
		max = ConnectionNumber(dpy);
		if (qt_hasaudio) {
		    if (use_alsa == 0) {
			FD_SET(oss_fd,&wr);
			if (oss_fd > max)
			    max = oss_fd;
		    }
		}

		if (qt_hasvideo) {
		    if(!has_frame) {
			if(0 != qt_frame_decode()) {
			    qt_hasvideo = 0;
			    wait.tv_sec  = 0;
			    wait.tv_usec = 1000;
			} else {
			has_frame = 1;
			}
		    }
			
		qt_frame_delay(&start,&wait);
		

		/* "wait" is the time, we would have to wait.
		   If it's longer, than 2 ms, we'll continue feeding the
		   soundcard. This prevents audio underruns for frames with a
		   VERY long duration */
		 if(wait.tv_sec || (wait.tv_usec > 2000)) {
		     wait.tv_sec  = 0;
		     wait.tv_usec = 2000;
		     blit_frame = 0;
		 } else
		     blit_frame = 1;
		    
	     } else {
		    wait.tv_sec  = 0;
		    wait.tv_usec = 1000;
		}
		rc = select(max+1,&rd,&wr,NULL,&wait);
		if (qt_hasaudio) {
		    if (use_alsa == 1) {
			if (0 != qt_alsa_audio_write()) qt_hasaudio = 0;
		    }
		    else if (FD_ISSET(oss_fd,&wr)) { 
			if (0 != qt_oss_audio_write()) qt_hasaudio = 0;
		    }
		}
		if (qt_hasvideo && 0 == rc && blit_frame) {
		    qt_frame_blit();
		    has_frame = 0;
		}
	    }
	if(!qt_hasvideo && !qt_hasaudio)
	  break;
	}
    }
    qt_cleanup();
    fprintf(stderr, "Decoded %lld samples\n",
            (long long)total_samples_decoded);    
    return 0;
}

