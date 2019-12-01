#include <array>
#include <cstdlib>
#include <clocale>
#include <libgen.h>
#include <unistd.h>
#include <iostream>
#include "notcurses.h"

static void usage(std::ostream& os, const char* name, int exitcode)
  __attribute__ ((noreturn));

void usage(std::ostream& o, const char* name, int exitcode){
  o << "usage: " << name << " files" << '\n';
  exit(exitcode);
}

int ncview(struct notcurses* nc, struct ncvisual* ncv, int* averr){
  struct ncplane* n = notcurses_stdplane(nc);
  int frame = 1;
  AVFrame* avf;
  while( (avf = ncvisual_decode(ncv, averr)) ){
    ncplane_cursor_move_yx(n, 0, 0);
    ncplane_printf(n, "Got frame %05d\u2026", frame);
    if(ncvisual_render(ncv)){
fprintf(stderr, "couldn't render, fuck!\n");
      return -1;
    }
    if(notcurses_render(nc)){
      return -1;
    }
    ++frame;
    sleep(100);
  }
  if(*averr == AVERROR_EOF){
    return 0;
  }
  return -1;
}

int main(int argc, char** argv){
  setlocale(LC_ALL, "");
  if(argc == 1){
    usage(std::cerr, argv[0], EXIT_FAILURE);
  }
  notcurses_options opts{};
  opts.outfp = stdout;
  auto nc = notcurses_init(&opts);
  if(nc == nullptr){
    return EXIT_FAILURE;
  }
  int dimy, dimx;
  notcurses_term_dim_yx(nc, &dimy, &dimx);
  auto ncp = notcurses_newplane(nc, dimy - 1, dimx, 1, 0, nullptr);
  if(ncp == nullptr){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  for(int i = 1 ; i < argc ; ++i){
    std::array<char, 128> errbuf;
    int averr;
    auto ncv = ncplane_visual_open(ncp, argv[i], &averr);
    if(ncv == nullptr){
      av_make_error_string(errbuf.data(), errbuf.size(), averr);
      notcurses_stop(nc);
      std::cerr << "Error opening " << argv[i] << ": " << errbuf.data() << std::endl;
      return EXIT_FAILURE;
    }
    if(ncview(nc, ncv, &averr)){
      av_make_error_string(errbuf.data(), errbuf.size(), averr);
      notcurses_stop(nc);
      std::cerr << "Error decoding " << argv[i] << ": " << errbuf.data() << std::endl;
      return EXIT_FAILURE;
    }
    ncvisual_destroy(ncv);
  }
  if(notcurses_stop(nc)){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
