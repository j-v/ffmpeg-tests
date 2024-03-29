// ffmpegTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


#include <math.h>

extern "C" {

#include <libavutil/opt.h>

#include <libavcodec/avcodec.h>

#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>

#include <libswscale/swscale.h>
}

//#pragma comment(lib, "dev\\lib\\avcodec.lib")

#define INBUF_SIZE 4096
#define AUDIO_INBUF_SIZE 20480
#define AUDIO_REFILL_THRESH 409


/*
 * Video encoding example
 */
static void video_encode_example(const char *filename, AVCodecID codec_id)
{
    AVCodec *codec;
    AVCodecContext *c= NULL;
    int i, ret, x, y, got_output;
    FILE *f;
    AVFrame *frame, *src_frame;
    AVPacket pkt;
    uint8_t endcode[] = { 0, 0, 1, 0xb7 };

    printf("Encode video file %s\n", filename);

    /* find the mpeg1 video encoder */
    codec = avcodec_find_encoder(codec_id);
    if (!codec) {
        fprintf(stderr, "Codec not found\n");
        exit(1);
    }

    c = avcodec_alloc_context3(codec);
    if (!c) {
        fprintf(stderr, "Could not allocate video codec context\n");
        exit(1);
    }
	

    /* put sample parameters */
    //c->bit_rate = 400000;
    /* resolution must be a multiple of two */
    /*c->width = 1920;
    c->height = 1080;*/
	c->width = 640;
    c->height = 480;
    /* frames per second */
	c->time_base.num=1;
	c->time_base.den=60;
    c->gop_size = 10; /* emit one intra frame every ten frames */
    c->max_b_frames=1;
    c->pix_fmt = AV_PIX_FMT_YUV420P;
	//c->pix_fmt = AV_PIX_FMT_RGB24;
	//c->pix_fmt = AV_PIX_FMT_GRAY8;

    if(codec_id == AV_CODEC_ID_H264)
        av_opt_set(c->priv_data, "preset", "slow", 0);

    /* open it */
    if (avcodec_open2(c, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        exit(1);
    }

    f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Could not open %s\n", filename);
        exit(1);
    }

    frame = avcodec_alloc_frame();
    if (!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }
    frame->format = c->pix_fmt;
    frame->width  = c->width;
    frame->height = c->height;
	/* the image can be allocated by any means and av_image_alloc() is
     * just the most convenient way if av_malloc() is to be used */
    ret = av_image_alloc(frame->data, frame->linesize, c->width, c->height,
                         c->pix_fmt, 32);

	// src frame
	AVPixelFormat src_format = AV_PIX_FMT_RGB24;
	src_frame = avcodec_alloc_frame();
    if (!src_frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }
	src_frame->format = src_format;
    src_frame->width  = c->width;
    src_frame->height = c->height;
	/* the image can be allocated by any means and av_image_alloc() is
     * just the most convenient way if av_malloc() is to be used */
    ret = av_image_alloc(src_frame->data, src_frame->linesize, c->width, c->height,
                         src_format, 32);

	// conversion context
	static struct SwsContext *img_convert_ctx;
	img_convert_ctx = sws_getContext(c->width, c->height, 
                        src_format, 
                       c->width, c->height,  c->pix_fmt, SWS_BICUBIC, 
                        NULL, NULL, NULL);

    
    if (ret < 0) {
        fprintf(stderr, "Could not allocate raw picture buffer\n");
        exit(1);
    }

    /* encode video */
	int num_frames = 1000;
    for(i=0;i<num_frames;i++) {
        av_init_packet(&pkt);
        pkt.data = NULL;    // packet data will be allocated by the encoder
        pkt.size = 0;

        fflush(stdout);
        /* prepare a dummy image */

		// RGB24
		uint8_t * rgb_data[AV_NUM_DATA_POINTERS];
		  for(y=0;y<c->height;y++) {
				for(x=0;x<c->width;x++) {
					//src_frame->data[0][y * frame->linesize[0] + x*3] = (char)((i % 30) * (255.0/30.0)) ;
					//src_frame->data[0][y * src_frame->linesize[0] + x*3] = rand() % 255 ; //r
					//src_frame->data[0][y * src_frame->linesize[0] + x*3+1] = rand() % 255 ; //g
					//src_frame->data[0][y * src_frame->linesize[0] + x*3+2] = rand() % 255 ; //b

					src_frame->data[0][y * src_frame->linesize[0] + x*3] = (x * y + i % 600) * (255.0/600.0) ; //r
					src_frame->data[0][y * src_frame->linesize[0] + x*3+1] = (x * y + i % 600) * (255.0/600.0) ; //g
					src_frame->data[0][y * src_frame->linesize[0] + x*3+2] = (x * y + i % 600) * (255.0/600.0) ; //b
				}
		  }

		  // convert rgb24 to yuv
		  sws_scale(img_convert_ctx, src_frame->data, 
              src_frame->linesize, 0, 
			  c->height, frame->data, frame->linesize);

		//// YUV
  //      /* Y */
  //      for(y=0;y<c->height;y++) {
  //          for(x=0;x<c->width;x++) {
  //              frame->data[0][y * frame->linesize[0] + x] = x + y + i * 3;
		//		//frame->data[0][y * frame->linesize[0] + x] = 
  //          }
  //      }

  //      /* Cb and Cr */
  //      for(y=0;y<c->height/2;y++) {
  //          for(x=0;x<c->width/2;x++) {
  //              frame->data[1][y * frame->linesize[1] + x] = 128 + y + i * 2;
  //              frame->data[2][y * frame->linesize[2] + x] = 64 + x + i * 5;
  //          }
  //      }

        frame->pts = i;

        /* encode the image */
        ret = avcodec_encode_video2(c, &pkt, frame, &got_output);
        if (ret < 0) {
            fprintf(stderr, "Error encoding frame\n");
            exit(1);
        }

        if (got_output) {
            printf("Write frame %3d (size=%5d)\n", i, pkt.size);
            fwrite(pkt.data, 1, pkt.size, f);
            av_free_packet(&pkt);
        }
    }

    /* get the delayed frames */
    for (got_output = 1; got_output; i++) {
        fflush(stdout);

        ret = avcodec_encode_video2(c, &pkt, NULL, &got_output);
        if (ret < 0) {
            fprintf(stderr, "Error encoding frame\n");
            exit(1);
        }

        if (got_output) {
            printf("Write frame %3d (size=%5d)\n", i, pkt.size);
            fwrite(pkt.data, 1, pkt.size, f);
            av_free_packet(&pkt);
        }
    }

    /* add sequence end code to have a real mpeg file */
    fwrite(endcode, 1, sizeof(endcode), f);
    fclose(f);

    avcodec_close(c);
    av_free(c);
    av_freep(&frame->data[0]);
    avcodec_free_frame(&frame);

	av_freep(&src_frame->data[0]);
    avcodec_free_frame(&src_frame);

    printf("\n");
}

/*
 * Video decoding example
 */



int _tmain(int argc, _TCHAR* argv[])
{
	avcodec_register_all();
	//video_encode_example("testfile.mpeg", AV_CODEC_ID_MPEG1VIDEO);
	video_encode_example("testfile.avi", AV_CODEC_ID_RAWVIDEO);
	return 0;
}

