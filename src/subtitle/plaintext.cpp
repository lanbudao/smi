#include "plaintext.h"
#include <cstring>

namespace PlainText {

struct buf {
    char *start;
    int size;
    int len;
};

static void append(struct buf *b, char c)
{
    if (b->len < b->size) {
        b->start[b->len] = c;
        b->len++;
    }
}

static void ass_to_plaintext(struct buf *b, const char *in)
{
    bool in_tag = false;
    const char *open_tag_pos = NULL;
    bool in_drawing = false;
    while (*in) {
        if (in_tag) {
            if (in[0] == '}') {
                in += 1;
                in_tag = false;
            }
            else if (in[0] == '\\' && in[1] == 'p') {
                in += 2;
                // Skip text between \pN and \p0 tags. A \p without a number
                // is the same as \p0, and leading 0s are also allowed.
                in_drawing = false;
                while (in[0] >= '0' && in[0] <= '9') {
                    if (in[0] != '0')
                        in_drawing = true;
                    in += 1;
                }
            }
            else {
                in += 1;
            }
        }
        else {
            if (in[0] == '\\' && (in[1] == 'N' || in[1] == 'n')) {
                in += 2;
                append(b, '\n');
            }
            else if (in[0] == '\\' && in[1] == 'h') {
                in += 2;
                append(b, ' ');
            }
            else if (in[0] == '{') {
                open_tag_pos = in;
                in += 1;
                in_tag = true;
            }
            else {
                if (!in_drawing)
                    append(b, in[0]);
                in += 1;
            }
        }
    }
    // A '{' without a closing '}' is always visible.
    if (in_tag) {
        while (*open_tag_pos)
            append(b, *open_tag_pos++);
    }
}

std::string fromAss(const char* ass) {
    char text[512];
    memset(text, 0, sizeof(text));
    struct buf b;
    b.start = text;
    b.size = sizeof(text) - 1;
    b.len = 0;
    ass_to_plaintext(&b, ass);
    int hour1, min1, sec1, hunsec1, hour2, min2, sec2, hunsec2;
    char line[512], *ret;
    // fixme: "\0" maybe not allowed
    if (sscanf(b.start, "Dialogue: Marked=%*d,%d:%d:%d.%d,%d:%d:%d.%d%[^\r\n]", //&nothing,
        &hour1, &min1, &sec1, &hunsec1,
        &hour2, &min2, &sec2, &hunsec2,
        line) < 9)
        if (sscanf(b.start, "Dialogue: %*d,%d:%d:%d.%d,%d:%d:%d.%d%[^\r\n]", //&nothing,
            &hour1, &min1, &sec1, &hunsec1,
            &hour2, &min2, &sec2, &hunsec2,
            line) < 9)
            return std::string(b.start); //libass ASS_Event.Text has no Dialogue
    ret = strchr(line, ',');
    if (!ret)
        return std::string(line);
    static const char kDefaultStyle[] = "Default,";
    for (int comma = 0; comma < 6; comma++) {
        if (!(ret = strchr(++ret, ','))) {
            // workaround for ffmpeg decoded srt in ass format: "Dialogue: 0,0:42:29.20,0:42:31.08,Default,Chinese\NEnglish.
            if (!(ret = strstr(line, kDefaultStyle))) {
                if (line[0] == ',') //work around for libav-9-
                    return std::string(line + 1);
                return std::string(line);
            }
            else {
                ret += sizeof(kDefaultStyle) - 1 - 1; // tail \0
            }
        }
    }
    ret++;
    int p = strcspn(b.start, "\r\n");
    if (p == b.len) //not found
        return std::string(ret);
    std::string line2 = std::string(b.start + p + 1);
    if (line2.empty())
        return std::string(ret);
    return std::string(ret) + "\n" + line2;
}
}
