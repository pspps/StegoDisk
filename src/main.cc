#include "carrier_files/carrier_file_ffmpeg.h"


void modifier(AVPacket * packet) {
	static int posByte = 0;
	static int posString = 0;
	const char string[] = "Test string!";

	if (posByte > sizeof(string))
		return;

	packet->pts = (packet->pts&(~1)) | ((string[posString]>>(7-posByte)) & 1);
	
	if (++posByte == 8) {
		posByte = 0;
		posString++;
	}
}

void modifier2(AVPacket * packet) {
	static int posByte = 0;
	static int posString = 0;
	static char string[500] = "";
	static bool finished = false;

	if (finished)
		return;

	string[posString] |= (packet->pts&1) << (7-posByte);
	
	if (++posByte == 8) {
		if (!string[posString]) {
			finished = true;
			printf("Data su '%s'\n", string);
		} else {
			posByte = 0;
			posString++;
		}
	}
}

int main() {
	{
		stego_disk::FfFormatContext contextIn{};
		contextIn.OpenInputFile("a.mp4");

		stego_disk::FfFormatContext contextOut{};
		contextOut.OpenOutputFile("b.mp4");

		contextOut.InitOutputFileFromFromInputFIle(contextIn);
		contextOut.CopyDataFromInputFileWithModifier(contextIn, modifier);
	}
	stego_disk::FfFormatContext context{};
	context.OpenInputFile("b.mp4");
	context.IterateOverStream(modifier2);
}
