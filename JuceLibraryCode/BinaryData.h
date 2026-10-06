/* =========================================================================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#pragma once

namespace BinaryData
{
    extern const char*   JetBrainsMono400_ttf;
    const int            JetBrainsMono400_ttfSize = 115152;

    extern const char*   JetBrainsMono500_ttf;
    const int            JetBrainsMono500_ttfSize = 115156;

    extern const char*   JetBrainsMono600_ttf;
    const int            JetBrainsMono600_ttfSize = 115088;

    extern const char*   JetBrainsMono700_ttf;
    const int            JetBrainsMono700_ttfSize = 115080;

    extern const char*   JetBrainsMono800_ttf;
    const int            JetBrainsMono800_ttfSize = 115028;

    extern const char*   OFL_txt;
    const int            OFL_txtSize = 4399;

    // Number of elements in the namedResourceList and originalFileNames arrays.
    const int namedResourceListSize = 6;

    // Points to the start of a list of resource names.
    extern const char* namedResourceList[];

    // Points to the start of a list of resource filenames.
    extern const char* originalFilenames[];

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding data and its size (or a null pointer if the name isn't found).
    const char* getNamedResource (const char* resourceNameUTF8, int& dataSizeInBytes);

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding original, non-mangled filename (or a null pointer if the name isn't found).
    const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8);
}
