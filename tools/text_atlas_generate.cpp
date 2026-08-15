/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Deterministic developer text atlas generator                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <chrono>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "text/text_atlas_product_internal.h"

#if defined(DVZ_HAS_MSDF_ATLAS) && DVZ_HAS_MSDF_ATLAS
#include <ft2build.h>
#include FT_FREETYPE_H
#endif



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

static constexpr uint32_t FIRST_ASCII_CODEPOINT = 32u;
static constexpr uint32_t LAST_ASCII_CODEPOINT = 126u;

#define DVZ_STRINGIFY_INNER(value) #value
#define DVZ_STRINGIFY(value) DVZ_STRINGIFY_INNER(value)



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct Options
{
    std::string primary_path;
    std::string fallback_path;
    std::filesystem::path output_dir;
    int32_t primary_face_index = 0;
    int32_t fallback_face_index = 0;
    DvzTextAtlasProductParams params = {};
};


struct ProductRecipe
{
    uint32_t em_px;
    uint32_t distance_range_px;
    const char* rgba_filename;
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Print the command-line contract.
 *
 * @param stream output stream
 */
static void print_usage(std::ostream& stream)
{
    stream
        << "Usage: datoviz_text_atlas_generate --primary FONT --output-dir DIR [OPTIONS]\n\n"
        << "Generate the canonical printable-ASCII 32/4, 64/8, and 128/16 MSDF products.\n"
        << "The output directory must not already contain product.json or atlas_*.rgba.\n\n"
        << "Options:\n"
        << "  --fallback FONT              Optional per-codepoint fallback font\n"
        << "  --primary-face-index N       Primary font face index (default: 0)\n"
        << "  --fallback-face-index N      Fallback font face index (default: 0)\n"
        << "  --fallback-codepoint N       Visible replacement codepoint (default: 63)\n"
        << "  --edge-coloring-seed N       Edge-coloring seed (default: 0)\n"
        << "  --max-corner-angle X         Edge-coloring angle (default: 3)\n"
        << "  --miter-limit X              Atlas packer miter limit (default: 1)\n"
        << "  --overlap-support 0|1        Enable overlap support (default: 1)\n"
        << "  --scanline-pass 0|1          Enable scanline correction (default: 1)\n"
        << "  --preprocess-geometry 0|1    Preprocess glyph geometry (default: 1)\n"
        << "  --enable-kerning 0|1         Load kerning data (default: 1)\n"
        << "  --help                       Show this help\n\n"
        << "Generation always uses one worker thread for canonical byte reproducibility.\n";
}



/**
 * Return the compiler identity recorded in the neutral product.
 *
 * @return compiler identity
 */
static const char* compiler_identity()
{
#if defined(__clang__)
    return "clang " __clang_version__;
#elif defined(_MSC_VER)
    return "msvc " DVZ_STRINGIFY(_MSC_FULL_VER);
#elif defined(__GNUC__)
    return "gcc " __VERSION__;
#else
    return "unknown";
#endif
}



/**
 * Return the configured target system name.
 *
 * @return target system name
 */
static const char* system_name()
{
#if defined(DVZ_TEXT_ATLAS_GENERATOR_SYSTEM_NAME)
    return DVZ_TEXT_ATLAS_GENERATOR_SYSTEM_NAME;
#else
    return "unknown";
#endif
}



/**
 * Return the configured target system processor.
 *
 * @return target processor
 */
static const char* system_processor()
{
#if defined(DVZ_TEXT_ATLAS_GENERATOR_SYSTEM_PROCESSOR)
    return DVZ_TEXT_ATLAS_GENERATOR_SYSTEM_PROCESSOR;
#else
    return "unknown";
#endif
}



/**
 * Return the msdf-atlas-gen dependency version.
 *
 * @return dependency version
 */
static const char* msdf_atlas_version()
{
#if defined(MSDF_ATLAS_VERSION)
    return DVZ_STRINGIFY(MSDF_ATLAS_VERSION);
#else
    return "unknown";
#endif
}



/**
 * Return the msdfgen dependency version.
 *
 * @return dependency version
 */
static const char* msdfgen_version()
{
#if defined(MSDFGEN_VERSION)
    return DVZ_STRINGIFY(MSDFGEN_VERSION);
#else
    return "unknown";
#endif
}



/**
 * Return the linked FreeType runtime version.
 *
 * @return dependency version
 */
static std::string freetype_version()
{
#if defined(DVZ_HAS_MSDF_ATLAS) && DVZ_HAS_MSDF_ATLAS
    FT_Library library = nullptr;
    if (FT_Init_FreeType(&library) != 0)
        return "unknown";
    FT_Int major = 0;
    FT_Int minor = 0;
    FT_Int patch = 0;
    FT_Library_Version(library, &major, &minor, &patch);
    FT_Done_FreeType(library);
    return std::to_string(major) + "." + std::to_string(minor) + "." +
           std::to_string(patch);
#else
    return "unavailable";
#endif
}



/**
 * Parse one signed 32-bit integer.
 *
 * @param value source string
 * @param option option name for diagnostics
 * @return parsed value
 */
static int32_t parse_i32(const std::string& value, const char* option)
{
    size_t consumed = 0;
    long long parsed = std::stoll(value, &consumed, 10);
    if (consumed != value.size() || parsed < 0 || parsed > INT32_MAX)
        throw std::runtime_error(std::string("invalid value for ") + option + ": " + value);
    return (int32_t)parsed;
}



/**
 * Parse one unsigned 64-bit integer.
 *
 * @param value source string
 * @param option option name for diagnostics
 * @return parsed value
 */
static uint64_t parse_u64(const std::string& value, const char* option)
{
    if (value.empty() || value[0] == '-')
        throw std::runtime_error(std::string("invalid value for ") + option + ": " + value);
    size_t consumed = 0;
    unsigned long long parsed = std::stoull(value, &consumed, 10);
    if (consumed != value.size())
        throw std::runtime_error(std::string("invalid value for ") + option + ": " + value);
    return (uint64_t)parsed;
}



/**
 * Parse one finite positive double.
 *
 * @param value source string
 * @param option option name for diagnostics
 * @return parsed value
 */
static double parse_positive_double(const std::string& value, const char* option)
{
    size_t consumed = 0;
    double parsed = std::stod(value, &consumed);
    if (consumed != value.size() || !(parsed > 0.0) || parsed > std::numeric_limits<double>::max())
        throw std::runtime_error(std::string("invalid value for ") + option + ": " + value);
    return parsed;
}



/**
 * Parse a canonical boolean encoded as zero or one.
 *
 * @param value source string
 * @param option option name for diagnostics
 * @return parsed value
 */
static bool parse_bool(const std::string& value, const char* option)
{
    if (value == "0")
        return false;
    if (value == "1")
        return true;
    throw std::runtime_error(std::string("invalid value for ") + option + ": " + value);
}



/**
 * Parse generator command-line options.
 *
 * @param argc argument count
 * @param argv argument vector
 * @return parsed options
 */
static Options parse_options(int argc, char** argv)
{
    Options options = {};
    options.params = _text_atlas_product_params_default();
    options.params.thread_count = 1;

    for (int i = 1; i < argc; i++)
    {
        std::string option = argv[i];
        if (option == "--help")
        {
            print_usage(std::cout);
            std::exit(0);
        }
        if (i + 1 >= argc)
            throw std::runtime_error("missing value for " + option);
        std::string value = argv[++i];
        if (option == "--primary")
            options.primary_path = value;
        else if (option == "--fallback")
            options.fallback_path = value;
        else if (option == "--output-dir")
            options.output_dir = value;
        else if (option == "--primary-face-index")
            options.primary_face_index = parse_i32(value, option.c_str());
        else if (option == "--fallback-face-index")
            options.fallback_face_index = parse_i32(value, option.c_str());
        else if (option == "--fallback-codepoint")
        {
            uint64_t parsed = parse_u64(value, option.c_str());
            if (parsed > UINT32_MAX)
                throw std::runtime_error("value out of range for " + option + ": " + value);
            options.params.fallback_codepoint = (uint32_t)parsed;
        }
        else if (option == "--edge-coloring-seed")
            options.params.edge_coloring_seed = parse_u64(value, option.c_str());
        else if (option == "--max-corner-angle")
            options.params.max_corner_angle = parse_positive_double(value, option.c_str());
        else if (option == "--miter-limit")
            options.params.miter_limit = parse_positive_double(value, option.c_str());
        else if (option == "--overlap-support")
            options.params.overlap_support = parse_bool(value, option.c_str());
        else if (option == "--scanline-pass")
            options.params.scanline_pass = parse_bool(value, option.c_str());
        else if (option == "--preprocess-geometry")
            options.params.preprocess_geometry = parse_bool(value, option.c_str());
        else if (option == "--enable-kerning")
            options.params.enable_kerning = parse_bool(value, option.c_str());
        else
            throw std::runtime_error("unknown option: " + option);
    }
    if (options.primary_path.empty())
        throw std::runtime_error("--primary is required");
    if (options.output_dir.empty())
        throw std::runtime_error("--output-dir is required");
    return options;
}



/**
 * Read an immutable font source into memory.
 *
 * @param path input path
 * @return complete file bytes
 */
static std::vector<uint8_t> read_file(const std::string& path)
{
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream)
        throw std::runtime_error("unable to open input font: " + path);
    std::streamoff end = stream.tellg();
    if (end <= 0 || (uint64_t)end > (uint64_t)SIZE_MAX)
        throw std::runtime_error("invalid input font size: " + path);
    std::vector<uint8_t> bytes((size_t)end);
    stream.seekg(0, std::ios::beg);
    stream.read((char*)bytes.data(), end);
    if (!stream)
        throw std::runtime_error("unable to read input font: " + path);
    return bytes;
}



/**
 * Escape one UTF-8 string for JSON without changing non-ASCII bytes.
 *
 * @param value source string
 * @return escaped JSON string contents
 */
static std::string json_escape(const std::string& value)
{
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    for (char raw : value)
    {
        unsigned char ch = (unsigned char)raw;
        switch (ch)
        {
        case '"':
            stream << "\\\"";
            break;
        case '\\':
            stream << "\\\\";
            break;
        case '\b':
            stream << "\\b";
            break;
        case '\f':
            stream << "\\f";
            break;
        case '\n':
            stream << "\\n";
            break;
        case '\r':
            stream << "\\r";
            break;
        case '\t':
            stream << "\\t";
            break;
        default:
            if (ch < 0x20)
            {
                stream << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (uint32_t)ch
                       << std::dec;
            }
            else
                stream << (char)ch;
            break;
        }
    }
    return stream.str();
}



/**
 * Return an exact hexadecimal encoding of a float's IEEE-754 bits.
 *
 * @param value float value
 * @return eight-digit hexadecimal bit string
 */
static std::string f32_bits(float value)
{
    uint32_t bits = 0;
    static_assert(sizeof(bits) == sizeof(value), "float32 is required");
    std::memcpy(&bits, &value, sizeof(bits));
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << "0x" << std::hex << std::setw(8) << std::setfill('0') << bits;
    return stream.str();
}



/**
 * Return a locale-independent round-trip double string.
 *
 * @param value double value
 * @return decimal string
 */
static std::string f64_decimal(double value)
{
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << std::setprecision(std::numeric_limits<double>::max_digits10) << value;
    return stream.str();
}



/**
 * Write one exact float array as JSON bit strings.
 *
 * @param stream output stream
 * @param values float values
 * @param count value count
 */
static void write_f32_array(std::ostream& stream, const float* values, uint32_t count)
{
    stream << '[';
    for (uint32_t i = 0; i < count; i++)
    {
        if (i > 0)
            stream << ',';
        stream << '"' << f32_bits(values[i]) << '"';
    }
    stream << ']';
}



/**
 * Append one product's deterministic metadata to the product JSON document.
 *
 * @param stream JSON output stream
 * @param recipe requested product recipe
 * @param product realized product
 * @param first whether this is the first product
 */
static void write_product_json(
    std::ostream& stream, const ProductRecipe& recipe, const DvzTextAtlasProduct* product,
    bool first)
{
    if (!first)
        stream << ",\n";
    stream << "    {\n";
    stream << "      \"requested\":{\"em_px_bits\":\"" << f32_bits((float)recipe.em_px)
           << "\",\"distance_range_px_bits\":\""
           << f32_bits((float)recipe.distance_range_px) << "\",\"flags\":0},\n";
    stream << "      \"backend\":{\"name\":\"msdf\",\"value\":" << (int)product->backend
           << "},\n";
    stream << "      \"encoding\":{\"name\":\"msdf_rgb\",\"value\":"
           << (int)product->encoding << "},\n";
    stream << "      \"dimensions\":{\"width\":" << product->width
           << ",\"height\":" << product->height << ",\"channels\":" << product->channels
           << "},\n";
    stream << "      \"metrics\":{\"em_px_bits\":\"" << f32_bits(product->em_px)
           << "\",\"distance_range_px_bits\":\"" << f32_bits(product->distance_range_px)
           << "\",\"ascent_bits\":\"" << f32_bits(product->ascent)
           << "\",\"descent_bits\":\"" << f32_bits(product->descent)
           << "\",\"line_gap_bits\":\"" << f32_bits(product->line_gap)
           << "\",\"line_height_bits\":\"" << f32_bits(product->line_height) << "\"},\n";
    stream << "      \"rgba\":{\"filename\":\"" << recipe.rgba_filename
           << "\",\"size\":" << product->rgba_size
           << ",\"pixel_format\":\"rgba8\",\"row_order\":\"top_to_bottom\"},\n";
    stream << "      \"glyph_count\":" << product->glyph_count
           << ",\"coverage_count\":" << product->coverage_count
           << ",\"fallback_mapping_count\":" << product->fallback_mapping_count << ",\n";
    stream << "      \"glyphs\":[\n";
    for (uint32_t i = 0; i < product->glyph_count; i++)
    {
        const DvzTextAtlasGlyph* glyph = &product->glyphs[i];
        stream << "        {\"codepoint\":" << glyph->codepoint << ",\"glyph_id\":"
               << glyph->glyph_id << ",\"advance_bits\":\"" << f32_bits(glyph->advance)
               << "\",\"xoff_bits\":\"" << f32_bits(glyph->xoff)
               << "\",\"yoff_bits\":\"" << f32_bits(glyph->yoff)
               << "\",\"width_bits\":\"" << f32_bits(glyph->width)
               << "\",\"height_bits\":\"" << f32_bits(glyph->height)
               << "\",\"plane_bounds_bits\":";
        write_f32_array(stream, glyph->plane_bounds, 4);
        stream << ",\"atlas_bounds_bits\":";
        write_f32_array(stream, glyph->atlas_bounds, 4);
        stream << ",\"uv_bits\":";
        write_f32_array(stream, glyph->uv, 4);
        stream << ",\"valid\":" << (glyph->valid ? "true" : "false") << '}';
        stream << (i + 1u < product->glyph_count ? ",\n" : "\n");
    }
    stream << "      ],\n";
    stream << "      \"coverage\":[\n";
    for (uint32_t i = 0; i < product->coverage_count; i++)
    {
        const DvzTextAtlasProductCoverage* item = &product->coverage[i];
        stream << "        {\"requested_codepoint\":" << item->requested_codepoint
               << ",\"resolved_codepoint\":" << item->resolved_codepoint
               << ",\"glyph_index\":" << item->glyph_index << ",\"kind\":\""
               << (item->kind == DVZ_TEXT_ATLAS_PRODUCT_COVERAGE_EXACT ? "exact" : "fallback")
               << "\",\"font_role\":\""
               << (item->font_role == DVZ_TEXT_ATLAS_PRODUCT_FONT_PRIMARY ? "primary" : "fallback")
               << "\"}";
        stream << (i + 1u < product->coverage_count ? ",\n" : "\n");
    }
    stream << "      ]\n";
    stream << "    }";
}



/**
 * Write bytes to a new file.
 *
 * @param path output path
 * @param bytes byte array
 * @param size byte count
 */
static void write_new_file(const std::filesystem::path& path, const void* bytes, uint64_t size)
{
    if (std::filesystem::exists(path))
        throw std::runtime_error("refusing to overwrite existing output: " + path.string());
    if (size > (uint64_t)std::numeric_limits<std::streamsize>::max())
        throw std::runtime_error("output is too large: " + path.string());
    std::ofstream stream(path, std::ios::binary | std::ios::out);
    if (!stream)
        throw std::runtime_error("unable to create output: " + path.string());
    stream.write((const char*)bytes, (std::streamsize)size);
    stream.close();
    if (!stream)
        throw std::runtime_error("unable to write output: " + path.string());
}



/**
 * Return the canonical printable-ASCII codepoint set.
 *
 * @return sorted codepoints U+0020 through U+007E
 */
static std::vector<uint32_t> ascii_codepoints()
{
    std::vector<uint32_t> codepoints;
    codepoints.reserve(LAST_ASCII_CODEPOINT - FIRST_ASCII_CODEPOINT + 1u);
    for (uint32_t codepoint = FIRST_ASCII_CODEPOINT; codepoint <= LAST_ASCII_CODEPOINT; codepoint++)
        codepoints.push_back(codepoint);
    return codepoints;
}



/**
 * Generate all canonical products and write the neutral intermediate.
 *
 * @param options generator options
 */
static void generate(const Options& options)
{
    if (!_text_atlas_product_msdf_available())
        throw std::runtime_error("MSDF atlas generation is not available in this build");

    static constexpr ProductRecipe recipes[] = {
        {32u, 4u, "atlas_32.rgba"},
        {64u, 8u, "atlas_64.rgba"},
        {128u, 16u, "atlas_128.rgba"},
    };
    std::filesystem::create_directories(options.output_dir);
    const std::filesystem::path manifest_path = options.output_dir / "product.json";
    if (std::filesystem::exists(manifest_path))
        throw std::runtime_error("refusing to overwrite existing output: " + manifest_path.string());
    for (const ProductRecipe& recipe : recipes)
    {
        std::filesystem::path path = options.output_dir / recipe.rgba_filename;
        if (std::filesystem::exists(path))
            throw std::runtime_error("refusing to overwrite existing output: " + path.string());
    }

    std::vector<uint8_t> primary_bytes = read_file(options.primary_path);
    std::vector<uint8_t> fallback_bytes;
    if (!options.fallback_path.empty())
        fallback_bytes = read_file(options.fallback_path);
    DvzTextAtlasFontView primary = {};
    primary.bytes = primary_bytes.data();
    primary.size = primary_bytes.size();
    primary.face_index = options.primary_face_index;
    DvzTextAtlasFontView fallback = {};
    fallback.bytes = fallback_bytes.empty() ? nullptr : fallback_bytes.data();
    fallback.size = fallback_bytes.size();
    fallback.face_index = options.fallback_face_index;
    DvzTextAtlasProductBudget budget = _text_atlas_product_budget_default();
    std::vector<uint32_t> codepoints = ascii_codepoints();

    std::ostringstream json;
    json.imbue(std::locale::classic());
    json << "{\n";
    json << "  \"schema_version\":1,\n";
    json << "  \"format\":\"datoviz_text_atlas_product\",\n";
    json << "  \"product\":\"default_msdf_atlas\",\n";
    json << "  \"generation\":{\n";
    json << "    \"tool\":\"datoviz_text_atlas_generate\",\"tool_version\":1,\n";
    json << "    \"command_template\":\"datoviz_text_atlas_generate --primary <font> "
            "[--fallback <font>] --output-dir <dir> [recipe options]\",\n";
    json << "    \"canonical_thread_count\":1,\n";
    json << "    \"dependencies\":{\"msdf_atlas_gen\":\""
         << json_escape(msdf_atlas_version()) << "\",\"msdfgen\":\""
         << json_escape(msdfgen_version()) << "\",\"freetype\":\""
         << json_escape(freetype_version()) << "\"},\n";
    json << "    \"build\":{\"compiler\":\"" << json_escape(compiler_identity())
         << "\",\"system\":\"" << json_escape(system_name()) << "\",\"processor\":\""
         << json_escape(system_processor()) << "\"}\n";
    json << "  },\n";
    json << "  \"sources\":{\n";
    json << "    \"primary\":{\"path\":\"" << json_escape(options.primary_path)
         << "\",\"byte_size\":" << primary.size << ",\"face_index\":"
         << primary.face_index << ",\"load_flags\":0},\n";
    if (!options.fallback_path.empty())
    {
        json << "    \"fallback\":{\"path\":\"" << json_escape(options.fallback_path)
             << "\",\"byte_size\":" << fallback.size << ",\"face_index\":"
             << fallback.face_index << ",\"load_flags\":0}\n";
    }
    else
        json << "    \"fallback\":null\n";
    json << "  },\n";
    json << "  \"glyph_set\":{\"name\":\"printable_ascii\",\"first_codepoint\":"
         << FIRST_ASCII_CODEPOINT << ",\"last_codepoint\":" << LAST_ASCII_CODEPOINT
         << ",\"count\":" << codepoints.size() << ",\"codepoints\":[";
    for (size_t i = 0; i < codepoints.size(); i++)
    {
        if (i > 0)
            json << ',';
        json << codepoints[i];
    }
    json << "]},\n";
    json << "  \"budget\":{\"max_glyphs\":" << budget.max_glyphs
         << ",\"max_dimension\":" << budget.max_dimension << ",\"max_rgba_bytes\":"
         << budget.max_rgba_bytes << "},\n";
    json << "  \"recipe\":{\"thread_count\":1,\"fallback_codepoint\":"
         << options.params.fallback_codepoint << ",\"edge_coloring_seed\":"
         << options.params.edge_coloring_seed << ",\"max_corner_angle\":\""
         << f64_decimal(options.params.max_corner_angle) << "\",\"miter_limit\":\""
         << f64_decimal(options.params.miter_limit) << "\",\"overlap_support\":"
         << (options.params.overlap_support ? "true" : "false") << ",\"scanline_pass\":"
         << (options.params.scanline_pass ? "true" : "false")
         << ",\"preprocess_geometry\":"
         << (options.params.preprocess_geometry ? "true" : "false")
         << ",\"enable_kerning\":" << (options.params.enable_kerning ? "true" : "false")
         << "},\n";
    json << "  \"products\":[\n";

    bool first = true;
    for (const ProductRecipe& recipe : recipes)
    {
        DvzTextAtlasSpec spec = {};
        spec.backend = DVZ_TEXT_ATLAS_BACKEND_MSDF;
        spec.em_px = (float)recipe.em_px;
        spec.distance_range_px = (float)recipe.distance_range_px;
        DvzTextAtlasProduct product = {};
        auto start = std::chrono::steady_clock::now();
        bool built = _text_atlas_product_build_msdf(
            &primary, fallback.bytes != nullptr ? &fallback : nullptr, &spec, codepoints.data(),
            (uint32_t)codepoints.size(), &budget, &options.params, &product);
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start);
        if (!built)
            throw std::runtime_error("MSDF product generation failed for em size " +
                                     std::to_string(recipe.em_px));
        try
        {
            write_new_file(
                options.output_dir / recipe.rgba_filename, product.rgba, product.rgba_size);
            write_product_json(json, recipe, &product, first);
            first = false;
            std::cout << "generated " << recipe.rgba_filename << ": " << product.width << 'x'
                      << product.height << ", " << product.glyph_count << " glyphs, "
                      << product.rgba_size << " bytes, " << product.fallback_mapping_count
                      << " fallback mappings, " << elapsed.count() << " ms\n";
        }
        catch (...)
        {
            _text_atlas_product_destroy(&product);
            throw;
        }
        _text_atlas_product_destroy(&product);
    }
    json << "\n  ]\n";
    json << "}\n";
    std::string json_bytes = json.str();
    write_new_file(manifest_path, json_bytes.data(), json_bytes.size());
    std::cout << "wrote " << manifest_path.string() << '\n';
}



/*************************************************************************************************/
/*  Entry point                                                                                  */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    try
    {
        generate(parse_options(argc, argv));
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "text atlas generation failed: " << error.what() << '\n';
        return 1;
    }
}
