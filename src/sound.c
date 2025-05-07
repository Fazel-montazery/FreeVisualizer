#include "sound.h"

snd_pcm_t *
alsa_open (int channels, unsigned samplerate, int realtime)
{	const char * device = "default" ;
	snd_pcm_t *alsa_dev = NULL ;
	snd_pcm_hw_params_t *hw_params ;
	snd_pcm_uframes_t buffer_size ;
	snd_pcm_uframes_t alsa_period_size, alsa_buffer_frames ;
	snd_pcm_sw_params_t *sw_params ;

	int err ;

	if (realtime)
	{	alsa_period_size = 256 ;
		alsa_buffer_frames = 3 * alsa_period_size ;
		}
	else
	{	alsa_period_size = 1024 ;
		alsa_buffer_frames = 4 * alsa_period_size ;
		} ;

	if ((err = snd_pcm_open (&alsa_dev, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0)
	{	fprintf (stderr, "cannot open audio device \"%s\" (%s)\n", device, snd_strerror (err)) ;
		goto catch_error ;
		} ;

	snd_pcm_nonblock (alsa_dev, 0) ;

	if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0)
	{	fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n", snd_strerror (err)) ;
		goto catch_error ;
		} ;

	if ((err = snd_pcm_hw_params_any (alsa_dev, hw_params)) < 0)
	{	fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n", snd_strerror (err)) ;
		goto catch_error ;
		} ;

	if ((err = snd_pcm_hw_params_set_access (alsa_dev, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
	{	fprintf (stderr, "cannot set access type (%s)\n", snd_strerror (err)) ;
		goto catch_error ;
		} ;

	if ((err = snd_pcm_hw_params_set_format (alsa_dev, hw_params, SND_PCM_FORMAT_FLOAT)) < 0)
	{	fprintf (stderr, "cannot set sample format (%s)\n", snd_strerror (err)) ;
		goto catch_error ;
		} ;

	if ((err = snd_pcm_hw_params_set_rate_near (alsa_dev, hw_params, &samplerate, 0)) < 0)
	{	fprintf (stderr, "cannot set sample rate (%s)\n", snd_strerror (err)) ;
		goto catch_error ;
		} ;

	if ((err = snd_pcm_hw_params_set_channels (alsa_dev, hw_params, channels)) < 0)
	{	fprintf (stderr, "cannot set channel count (%s)\n", snd_strerror (err)) ;
		goto catch_error ;
		} ;

	if ((err = snd_pcm_hw_params_set_buffer_size_near (alsa_dev, hw_params, &alsa_buffer_frames)) < 0)
	{	fprintf (stderr, "cannot set buffer size (%s)\n", snd_strerror (err)) ;
		goto catch_error ;
		} ;

	if ((err = snd_pcm_hw_params_set_period_size_near (alsa_dev, hw_params, &alsa_period_size, 0)) < 0)
	{	fprintf (stderr, "cannot set period size (%s)\n", snd_strerror (err)) ;
		goto catch_error ;
		} ;

	if ((err = snd_pcm_hw_params (alsa_dev, hw_params)) < 0)
	{	fprintf (stderr, "cannot set parameters (%s)\n", snd_strerror (err)) ;
		goto catch_error ;
		} ;

	/* extra check: if we have only one period, this code won't work */
	snd_pcm_hw_params_get_period_size (hw_params, &alsa_period_size, 0) ;
	snd_pcm_hw_params_get_buffer_size (hw_params, &buffer_size) ;
	if (alsa_period_size == buffer_size)
	{	fprintf (stderr, "Can't use period equal to buffer size (%lu == %lu)", alsa_period_size, buffer_size) ;
		goto catch_error ;
		} ;

	snd_pcm_hw_params_free (hw_params) ;

	if ((err = snd_pcm_sw_params_malloc (&sw_params)) != 0)
	{	fprintf (stderr, "%s: snd_pcm_sw_params_malloc: %s", __func__, snd_strerror (err)) ;
		goto catch_error ;
		} ;

	if ((err = snd_pcm_sw_params_current (alsa_dev, sw_params)) != 0)
	{	fprintf (stderr, "%s: snd_pcm_sw_params_current: %s", __func__, snd_strerror (err)) ;
		goto catch_error ;
		} ;

	/* note: set start threshold to delay start until the ring buffer is full */
	snd_pcm_sw_params_current (alsa_dev, sw_params) ;

	if ((err = snd_pcm_sw_params_set_start_threshold (alsa_dev, sw_params, buffer_size)) < 0)
	{	fprintf (stderr, "cannot set start threshold (%s)\n", snd_strerror (err)) ;
		goto catch_error ;
		} ;

	if ((err = snd_pcm_sw_params (alsa_dev, sw_params)) != 0)
	{	fprintf (stderr, "%s: snd_pcm_sw_params: %s", __func__, snd_strerror (err)) ;
		goto catch_error ;
		} ;

	snd_pcm_sw_params_free (sw_params) ;

	snd_pcm_reset (alsa_dev) ;

catch_error :

	if (err < 0 && alsa_dev != NULL)
	{	snd_pcm_close (alsa_dev) ;
		return NULL ;
		} ;

	return alsa_dev ;
} /* alsa_open */

int
alsa_write_float (snd_pcm_t *alsa_dev, float *data, int frames, int channels)
{	static	int epipe_count = 0 ;

	int total = 0 ;
	int retval ;

	if (epipe_count > 0)
		epipe_count -- ;

	while (total < frames)
	{	retval = snd_pcm_writei (alsa_dev, data + total * channels, frames - total) ;

		if (retval >= 0)
		{	total += retval ;
			if (total == frames)
				return total ;

			continue ;
			} ;

		switch (retval)
		{	case -EAGAIN :
					puts ("alsa_write_float: EAGAIN") ;
					continue ;
					break ;

			case -EPIPE :
					if (epipe_count > 0)
					{	printf ("alsa_write_float: EPIPE %d\n", epipe_count) ;
						if (epipe_count > 140)
							return retval ;
						} ;
					epipe_count += 100 ;

#if 0
					if (0)
					{	snd_pcm_status_t *status ;

						snd_pcm_status_alloca (&status) ;
						if ((retval = snd_pcm_status (alsa_dev, status)) < 0)
							fprintf (stderr, "alsa_out: xrun. can't determine length\n") ;
						else if (snd_pcm_status_get_state (status) == SND_PCM_STATE_XRUN)
						{	struct timeval now, diff, tstamp ;

							gettimeofday (&now, 0) ;
							snd_pcm_status_get_trigger_tstamp (status, &tstamp) ;
							timersub (&now, &tstamp, &diff) ;

							fprintf (stderr, "alsa_write_float xrun: of at least %.3f msecs. resetting stream\n",
									diff.tv_sec * 1000 + diff.tv_usec / 1000.0) ;
							}
						else
							fprintf (stderr, "alsa_write_float: xrun. can't determine length\n") ;
						} ;
#endif

					snd_pcm_prepare (alsa_dev) ;
					break ;

			case -EBADFD :
					fprintf (stderr, "alsa_write_float: Bad PCM state.n") ;
					return 0 ;
					break ;

#if defined ESTRPIPE && ESTRPIPE != EPIPE
			case -ESTRPIPE :
					fprintf (stderr, "alsa_write_float: Suspend event.n") ;
					return 0 ;
					break ;
#endif

			case -EIO :
					puts ("alsa_write_float: EIO") ;
					return 0 ;

			default :
					fprintf (stderr, "alsa_write_float: retval = %d\n", retval) ;
					return 0 ;
					break ;
			} ; /* switch */
		} ; /* while */

	return total ;
} /* alsa_write_float */
