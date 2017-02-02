#ifndef	__FFT_APP_H__
#define	__FFT_APP_H__

struct fft_app {
	pthread_mutex_t mtx_histogram;
	SDL_Surface *screen;
	SDL_TimerID rendering_timer_id;
	int g_do_update;
	TTF_Font *font;
	struct fft_display *fdisp;
	struct fft_histogram *fh;
	int highlight_freq;
	int startfreq;
	int accel;
	int scroll;
};


#endif
