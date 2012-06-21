#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "harness.h"
#include "fpga.h"

#define SND_BUF_SIZE 400

uint32_t adc_min[] = {430, 350, 250, 150};
uint32_t adc_max[] = {550, 480, 325, 200};

#ifdef linux
#include <alsa/asoundlib.h>
// 44100 = 440 * 100 = 220 * 200 (200 samples high, 200 samples low)
int test_audio(void) {
	int i;
	int adc, val;
	int err;
	short buf[SND_BUF_SIZE];
	unsigned int rate;

	snd_pcm_t *pcm;
	snd_pcm_hw_params_t *hw_params;

	for (i=0; i<(SND_BUF_SIZE/2); i++)
		buf[i] = SHRT_MAX;
	for (i=(SND_BUF_SIZE/2); i<SND_BUF_SIZE; i++)
		buf[i] = 0;

	if ((err = snd_pcm_open (&pcm, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
		harness_error(0, "Cannot open default audio device (%s)",
			 snd_strerror(err));
		return 1;
	}
	   
	if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
		harness_error(1, "Cannot allocate hardware parameter structure (%s)",
			 snd_strerror(err));
		return 1;
	}
			 
	if ((err = snd_pcm_hw_params_any (pcm, hw_params)) < 0) {
		harness_error(2, "cannot initialize hardware parameter structure (%s)",
			 snd_strerror(err));
		return 1;
	}

	if ((err = snd_pcm_hw_params_set_access (pcm, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		harness_error(3, "cannot set access type (%s)",
			 snd_strerror(err));
		return 1;
	}

	if ((err = snd_pcm_hw_params_set_format (pcm, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
		harness_error(4, "cannot set sample format (%s)",
			 snd_strerror(err));
		return 1;
	}

	rate = 44100;
	if ((err = snd_pcm_hw_params_set_rate_near (pcm, hw_params, &rate, 0)) < 0) {
		harness_error(5, "cannot set sample rate (%s)",
			 snd_strerror(err));
		return 1;
	}

	if ((err = snd_pcm_hw_params_set_channels (pcm, hw_params, 2)) < 0) {
		harness_error(6, "cannot set channel count (%s)",
			 snd_strerror(err));
		return 1;
	}

	if ((err = snd_pcm_hw_params (pcm, hw_params)) < 0) {
		harness_error(7, "cannot set parameters (%s)",
			 snd_strerror(err));
		return 1;
	}

	snd_pcm_hw_params_free (hw_params);

	if ((err = snd_pcm_prepare (pcm)) < 0) {
		harness_error(8, "cannot prepare audio interface for use (%s)",
			 snd_strerror(err));
		return 1;
	}


	for (adc=8; adc <= 11; adc++) {
		for (i=0; i<50; i++) {
			if ((err = snd_pcm_writei(pcm, buf, SND_BUF_SIZE)) != SND_BUF_SIZE) {
				harness_error(9, "write to audio interface failed (%s)",
					 snd_strerror(err));
				return 1;
			}
		}
		
		usleep(200000);
		val = read_adc(adc);

		if (val < adc_min[adc-8] || val > adc_max[adc-8])
			harness_error(adc, "ADC %d value incorrect.  Wanted between %d and %d, got : %d", adc, adc_min[adc-8], adc_max[adc-8], val);
		else
			harness_info(adc, "ADC %d value nominal: %d", adc, val);
	}

	snd_pcm_drain(pcm);
	snd_pcm_close(pcm);
	return 0;
}
#else
int test_audio(void) {
	return 0;
}
#endif //linux
