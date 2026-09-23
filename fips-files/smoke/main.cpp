// Verifies static linking, Arabic/Persian shaping, rasterization and Unicode layout.
#include <hb.h>
#include <hb-ft.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <unicode/ubidi.h>
#include <unicode/ubrk.h>
#include <unicode/uclean.h>
#include <unicode/unum.h>
#include <cstdio>
#include <cstdlib>

// Fail explicitly in both Debug and Release builds.
void Require(bool _condition, const char* _message)
{
    if (!_condition)
    {
        std::fprintf(stderr, "FAIL: %s\n", _message);
        std::exit(EXIT_FAILURE);
    }
}

// Exercise contextual Arabic glyphs and render the actual shaped glyph IDs.
void CheckShaping(FT_Face _face, const char* _text, const char* _language)
{
    hb_font_t* font = hb_ft_font_create_referenced(_face);
    hb_buffer_t* buffer = hb_buffer_create();
    hb_buffer_add_utf8(buffer, _text, -1, 0, -1);
    hb_buffer_set_direction(buffer, HB_DIRECTION_RTL);
    hb_buffer_set_script(buffer, HB_SCRIPT_ARABIC);
    hb_buffer_set_language(buffer, hb_language_from_string(_language, -1));
    const auto first_character = hb_buffer_get_glyph_infos(buffer, nullptr)[0].codepoint;
    const auto nominal_glyph = FT_Get_Char_Index(_face, first_character);
    hb_shape(font, buffer, nullptr, 0);

    unsigned count = 0;
    const auto* glyphs = hb_buffer_get_glyph_infos(buffer, &count);
    Require(count > 0, "shaping returned glyphs");
    bool has_pixels = false;
    bool has_contextual_form = false;

    for (unsigned index = 0; index < count; ++index)
    {
        Require(glyphs[index].codepoint != 0, "font covers every shaped glyph");
        Require(FT_Load_Glyph(_face, glyphs[index].codepoint, FT_LOAD_RENDER) == 0,
            "FreeType renders HarfBuzz glyph IDs");
        has_pixels |= _face->glyph->bitmap.width > 0;
        has_contextual_form |= glyphs[index].cluster == 0 && glyphs[index].codepoint != nominal_glyph;

        if (index > 0)
        {
            Require(glyphs[index].cluster <= glyphs[index - 1].cluster, "RTL cluster order");
        }
    }

    Require(has_contextual_form, "initial letter uses a joined contextual form");
    Require(has_pixels, "rasterized text contains visible pixels");
    hb_buffer_destroy(buffer);
    hb_font_destroy(font);
}

// Check that ICU data works without an installed ICU or an external data path.
void CheckUnicode()
{
    UErrorCode status = U_ZERO_ERROR;
    u_init(&status);
    Require(U_SUCCESS(status), "embedded ICU data initializes");

    const char16_t mixed[] = u"\u0633\u0644\u0627\u0645 Player 123";
    UBiDi* bidi = ubidi_open();
    ubidi_setPara(bidi, mixed, -1, UBIDI_DEFAULT_RTL, nullptr, &status);
    Require(U_SUCCESS(status) && ubidi_countRuns(bidi, &status) >= 2,
        "mixed Arabic, Latin and numbers have multiple directional runs");
    ubidi_close(bidi);

    const char16_t graphemes[] = u"a\u0301b";
    UBreakIterator* breaks = ubrk_open(UBRK_CHARACTER, "fa", graphemes, 3, &status);
    Require(U_SUCCESS(status) && breaks != nullptr, "Persian character iterator loads");
    Require(ubrk_first(breaks) == 0 && ubrk_next(breaks) == 2 && ubrk_next(breaks) == 3,
        "combining marks stay with their base character");
    ubrk_close(breaks);

    breaks = ubrk_open(UBRK_LINE, "ar", mixed, -1, &status);
    Require(U_SUCCESS(status) && breaks != nullptr, "Arabic line break data loads");
    Require(ubrk_next(breaks) != UBRK_DONE, "line break boundaries exist");
    ubrk_close(breaks);

    UNumberFormat* number = unum_open(UNUM_DECIMAL, nullptr, 0, "fa", nullptr, &status);
    UChar formatted[32] = {};
    const auto length = unum_format(number, 1234, formatted, 32, nullptr, &status);
    Require(U_SUCCESS(status) && length > 0 && formatted[0] == 0x06f1,
        "Persian locale uses Persian digits");
    unum_close(number);
    u_cleanup();
}

// Load the repository's test font and run the same checks on Windows and Linux.
int main(int _argc, char** _argv)
{
    Require(_argc == 2, "pass the Amiri test font path");
    FT_Library library = nullptr;
    FT_Face face = nullptr;
    Require(FT_Init_FreeType(&library) == 0, "FreeType initializes");
    Require(FT_New_Face(library, _argv[1], 0, &face) == 0, "test font opens");
    Require(FT_Set_Pixel_Sizes(face, 0, 24) == 0, "font size is set");

    CheckShaping(face, u8"\u0633\u0644\u0627\u0645", "ar");
    CheckShaping(face, u8"\u067e\u0627\u0631\u0633\u06cc", "fa");
    FT_Done_Face(face);
    FT_Done_FreeType(library);
    CheckUnicode();
    std::puts("PASS: Arabic/Persian shaping, glyph rasterization, bidi, breaks and localized numbers");
    return EXIT_SUCCESS;
}
