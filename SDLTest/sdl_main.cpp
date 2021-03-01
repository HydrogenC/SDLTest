#include <SDL.h>
#include <iostream>
#include <string>
#include <mkl.h>
#include <chrono>
#include <numeric>
#include "AudioFile.h"
#include <Windows.h>
#pragma comment(lib,"winmm.lib")
using namespace std;

inline SDL_Rect GetButtomRect(int ww, int wh, int x, int w, int h)
{
	return SDL_Rect{ .x = x, .y = wh - h, .w = w, .h = h };
}

unique_ptr<double[]> do_fft(double* arr, int sz)
{
	auto in = new MKL_Complex16[sz];
	auto out = new MKL_Complex16[sz];
	for (int i = 0; i < sz; i++)
	{
		in[i] = { .real = arr[i], .imag = 0 };
	}
	DFTI_DESCRIPTOR_HANDLE data_hand_;
	MKL_LONG status;
	status = DftiCreateDescriptor(&data_hand_, DFTI_DOUBLE, DFTI_COMPLEX, 1, sz);
	status = DftiSetValue(data_hand_, DFTI_PLACEMENT, DFTI_NOT_INPLACE);
	status = DftiCommitDescriptor(data_hand_);
	status = DftiComputeForward(data_hand_, in, out);
	status = DftiFreeDescriptor(&data_hand_);

	delete[] in;
	auto ret = make_unique<double[]>(sz);

	for (int i = 0; i < sz; i++)
	{
		ret[i] = out[i].real;
	}

	delete[] out;
	return ret;
}

int main(int argc, char* argv[])
{
	locale::global(locale(""));
	ios::sync_with_stdio(false);
	AudioFile<double> audioFile;
	cout << "Input audio file name: ";
	string str;
	getline(cin, str);
	audioFile.load(str);

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

	SDL_Window* window = SDL_CreateWindow("Spectrum", 300, 300, 640, 480, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	SDL_Renderer* render = SDL_CreateRenderer(window, -1, 0);
	SDL_Event sdl_event;
	SDL_Rect rct = { .x = 20,.y = 20,.w = 100,.h = 100 };


	auto start = SDL_GetTicks();
	PlaySoundA(str.c_str(), NULL, SND_FILENAME | SND_ASYNC);

	auto bfr = make_unique<double[]>(20);
	do
	{
		SDL_PollEvent(&sdl_event);
		switch (sdl_event.type)
		{
		case SDL_QUIT:
			goto __EXIT;
			break;
		default:
			break;
		}

		SDL_SetRenderDrawColor(render, 0, 0, 0, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(render);

		int start_sample = (SDL_GetTicks() - start) / 1000.0 * audioFile.getSampleRate();
		auto smp = do_fft(&audioFile.samples[0][0] + start_sample, 20000);

		int ww, wh;
		SDL_GetWindowSize(window, &ww, &wh);
		SDL_SetRenderDrawColor(render, 255, 0, 0, SDL_ALPHA_OPAQUE);
		for (int i = 0; i < 20; i++)
		{
			bfr[i] = 0.975 * bfr[i] + 0.025 * accumulate(smp.get() + i * 500, smp.get() + (i + 1) * 500, 0.0);
		}

		for (int i = 0; i < 20; i++)
		{
			if (bfr[i] < 10)
			{
				bfr[i] += (((i > 0 ? bfr[i - 1] : 0) + (i < 9 ? bfr[i + 1] : 0)) / 2 - bfr[i]) * 0.25;
			}
			SDL_Rect rct = GetButtomRect(ww, wh, i * ww / 20, ww / 24, 40 * sqrt(bfr[i]));
			SDL_RenderFillRect(render, &rct);
		}

		SDL_RenderPresent(render);
		Sleep(40);
	} while (true);

__EXIT:
	SDL_DestroyWindow(window);
	SDL_DestroyRenderer(render);
	return 0;
}