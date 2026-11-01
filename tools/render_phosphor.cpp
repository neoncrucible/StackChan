// SPDX-License-Identifier: MIT
// g++ -std=c++20 -Ifirmware/main tools/render_phosphor.cpp -o /tmp/phosphor-preview
// /tmp/phosphor-preview > docs/phosphor-avatar.svg
#include <phosphor/face.h>
#include <cstdio>

int main() {
    using namespace phosphor;
    std::puts("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"1120\" height=\"400\" viewBox=\"0 0 1120 400\">");
    std::puts("<rect width=\"1120\" height=\"400\" fill=\"#050a08\"/>");
    std::puts("<g fill=\"#a4ff7c\" font-family=\"monospace\"><text x=\"32\" y=\"40\" font-size=\"22\">PHOSPHOR</text><text x=\"32\" y=\"65\" font-size=\"12\" fill=\"#779783\">Original native face / 320 x 240 / no image assets</text></g>");
    const char* states[]{"READY", "LISTEN", "VOICE"};
    const char* captions[]{"A quiet moment.", "I'm listening.", "Right, let's get on with it."};
    for (int i = 0; i < 3; ++i) {
        std::printf("<g transform=\"translate(%d 96)\">", 32 + i * 368);
        std::printf("<rect width=\"320\" height=\"240\" fill=\"#%06x\" stroke=\"#244331\"/>", background);
        std::printf("<g fill=\"#a4ff7c\" font-family=\"monospace\" font-size=\"8\"><text x=\"20\" y=\"26\">PHOSPHOR // 01</text><text x=\"216\" y=\"26\">%s</text></g>", states[i]);
        std::puts("<g transform=\"translate(32 38)\">");
        Face f;
        f.state = i == 0 ? State::Ready : i == 1 ? State::Listening : State::Speaking;
        f.look_x = i == 0 ? -20 : 0;
        f.mouth = i == 2 ? 85 : 0;
        draw_face(f, [](int x, int y, int w, int h, uint32_t color) {
            std::printf("<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" fill=\"#%06x\"/>", x,y,w,h,color);
        });
        std::puts("</g>");
        std::printf("<text x=\"160\" y=\"210\" text-anchor=\"middle\" font-family=\"sans-serif\" font-size=\"16\" fill=\"#a4ff7c\">%s</text></g>", captions[i]);
    }
    std::puts("<text x=\"32\" y=\"377\" font-family=\"monospace\" font-size=\"12\" fill=\"#779783\">Shared firmware geometry. Speech mouth follows speaker PCM; the preview shows a representative level.</text></svg>");
}
