#ifndef PLAIN_TEXT_H
#define PLAIN_TEXT_H

#include <string>

namespace PlainText {

/*
 * Copy and modify from QtAV
 * from mpv/sub/sd_ass.c
 * ass_to_plaintext() was written by wm4 and he says it can be under LGPL
 */
std::string fromAss(const char* ass);

}

#endif
