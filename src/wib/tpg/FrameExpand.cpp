/**
 * @file WIBFrameProcessor.hpp WIB specific Task based raw processor
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "FrameExpand.hpp"

namespace swtpg {

//==============================================================================
// Print a 256-bit register interpreting it as packed 8-bit values
void print256(__m256i var)
{
    uint8_t *val = (uint8_t*) &var;
    for(int i=0; i<32; ++i) printf("%02x ", val[i]);
}

//==============================================================================
// Print a 256-bit register interpreting it as packed 8-bit values, in reverse order
void printr256(__m256i var)
{
    uint8_t *val = (uint8_t*) &var;
    for(int i=31; i>=0; --i) printf("%02x ", val[i]);
}

//==============================================================================
// Print a 256-bit register interpreting it as packed 16-bit values
void print256_as16(__m256i var)
{
    uint16_t *val = (uint16_t*) &var;
    for(int i=0; i<16; ++i) printf("%04x ", val[i]);
}

//==============================================================================
// Print a 256-bit register interpreting it as packed 16-bit values
void print256_as16_dec(__m256i var)
{
    int16_t *val = (int16_t*) &var;
    for(int i=0; i<16; ++i) printf("%+6d ", val[i]);
}

//==============================================================================
// Abortive attempt at expanding just the collection channels, instead
// of expanding all channels and then picking out just the collection
// ones.
RegisterArray<2> expand_segment_collection(const dunedaq::dataformats::ColdataBlock& __restrict__ block)
{
    const __m256i* __restrict__ coldata_start=reinterpret_cast<const __m256i*>(&block.segments[0]); // NOLINT
    __m256i raw0=_mm256_lddqu_si256(coldata_start+0);
    __m256i raw1=_mm256_lddqu_si256(coldata_start+1);
    __m256i raw2=_mm256_lddqu_si256(coldata_start+2);
    printr256(raw0); printf("  raw0\n");
    printr256(raw1); printf("  raw1\n");
    printr256(raw2); printf("  raw2\n");

    // For the first output item, we want these bytes:
    //
    // From raw0: 7, 9, 11, 13, 15, 17, 19, 21, 23, 24, 26, 28, 30
    // From raw1: 0, 2, 4, 6, 8
    //
    // They're non-overlapping so we can blend them
    //
    // So we get a blend mask of (where "x" means "don't care"):
    //
    // 31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
    // -------------------------------------------------------------------------------------
    //  x  0  x  0  x  0  x  0  0  x  0  x  0  x  0  x  0  x  0  x  0  x 0 1 0 1 x 1 x 1 x 1
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverflow"
    const __m256i blend_mask0=_mm256_set_epi8(0xdd, 0x00, 0xdd, 0x00, 0xdd, 0x00, 0xdd, 0x00,
                                              0x00, 0xdd, 0x00, 0xdd, 0x00, 0xdd, 0x00, 0xdd,
                                              0x00, 0xdd, 0x00, 0xdd, 0x00, 0xdd, 0x00, 0xff,
                                              0x00, 0xff, 0xdd, 0xff, 0xdd, 0xff, 0xdd, 0xff);
    // For the second output item, we want these bytes:
    //
    // From raw1: 23, 25, 27, 29, 31
    // From raw2: 1, 3, 5, 7, 8, 10, 12, 14, 16, 18, 20, 22, 24
    //
    // They're non-overlapping so we can blend them
    //
    // So we get a blend mask of (where "x" means "don't care"):
    //
    // 31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
    // -------------------------------------------------------------------------------------
    //  0  x  0  x  0  x  0  1  0  1  x  1  x  1  x  1  x  1  x  1  x  1 x 1 1 x 1 x 1 x 1 x
    const __m256i blend_mask1=_mm256_set_epi8(0x00, 0xdd, 0x00, 0xdd, 0x00, 0xdd, 0x00, 0xff,
                                              0x00, 0xff, 0xdd, 0xff, 0xdd, 0xff, 0xdd, 0xff,
                                              0xdd, 0xff, 0xdd, 0xff, 0xdd, 0xff, 0xdd, 0xff,
                                              0xff, 0xdd, 0xff, 0xdd, 0xff, 0xdd, 0xff, 0xdd);
#pragma GCC diagnostic pop
    __m256i out0=_mm256_blendv_epi8(raw0, raw1, blend_mask0);
    __m256i out1=_mm256_blendv_epi8(raw1, raw2, blend_mask1);
    printr256(out0); printf("  out0\n");
    printr256(out1); printf("  out1\n");

    // With the collection channels set to 0xcc and induction set to 0xff:
    // 31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
    // -----------------------------------------------------------------------------------------------
    // ff cc ff cc ff cc ff cc cc ff cc ff cc ff cc ff cc ff cc ff cc ff cc ff cc ff ff ff ff ff ff ff   raw0
    // cc ff cc ff cc ff cc ff cc ff ff ff ff ff ff ff ff ff ff ff ff ff ff cc ff cc ff cc ff cc ff cc   raw1
    // ff ff ff ff ff ff ff cc ff cc ff cc ff cc ff cc ff cc ff cc ff cc ff cc cc ff cc ff cc ff cc ff   raw2
    // cc cc cc cc cc cc cc cc cc ff cc ff cc ff cc ff cc ff cc ff cc ff cc cc cc cc ff cc ff cc ff cc   out0
    // cc ff cc ff cc ff cc cc cc cc ff cc ff cc ff cc ff cc ff cc ff cc ff cc cc ff cc ff cc ff cc ff   out1

    // Same with the "don't care"s blanked out: 
    // 31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
    // -----------------------------------------------------------------------------------------------
    //  x  0  x  0  x  0  x  0  0  x  0  x  0  x  0  x  0  x  0  x  0  x  0  1  0  1  x  1  x  1  x  1   blend_mask0
    //    cc    cc    cc    cc cc    cc    cc    cc    cc    cc    cc    cc cc cc cc    cc    cc    cc   out0

    // 31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
    // -----------------------------------------------------------------------------------------------
    //  0  x  0  x  0  x  0  1  0  1  x  1  x  1  x  1  x  1  x  1  x  1  x  1  1  x  1  x  1  x  1  x   blend_mask1
    // cc    cc    cc    cc cc cc cc    cc    cc    cc    cc    cc    cc    cc cc    cc    cc    cc      out1

    // So only 0xcc's remain. That's good. Next, set the first five
    // collection channels to 0x210, 0x543, 0x876, 0xba9, 0xedc to see where
    // they are:

    // 31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
    // -----------------------------------------------------------------------------------------------
    // ff cc ff cc ff cc ff cc cc ff ce ff dc ff ba ff 98 ff 76 ff 54 ff 32 ff 10 ff ff ff ff ff ff ff   raw0
    // cc cc cc cc cc cc cc cc cc ff ce ff dc ff ba ff 98 ff 76 ff 54 ff 32 cc 10 cc ff cc ff cc ff cc   out0


    // ...then I got bored trying to work out the shuffle needed, but
    // it's possible in principle
    return RegisterArray<2>();
}


namespace {
    // Lightly-edited otuput of  number_collection_and_induction

    // Collection:
    // min_channel is 1792 ie, offline 9472
    // Map from register position to offline offset relative to minimum collection channel in register array:
    const int collection_offlines[96] = {
         12,    14,    16,    18,    23,    21,    20,    22,    19,    17,    15,    13,   264,   266,   268,   270,
          0,     2,     4,     6,    11,     9,     8,    10,     7,     5,     3,     1,   275,   273,   272,   274,
         24,    26,    28,    30,    35,    33,    32,    34,    31,    29,    27,    25,   271,   269,   267,   265,
         36,    38,    40,    42,    47,    45,    44,    46,    43,    41,    39,    37,   276,   278,   280,   282,
        252,   254,   256,   258,   263,   261,   260,   262,   259,   257,   255,   253,   287,   285,   284,   286,
        240,   242,   244,   246,   251,   249,   248,   250,   247,   245,   243,   241,   283,   281,   279,   277
    };
    
    // Map from register position to online channel number within (crate,fiber,slot):
    const int collection_index_to_chan[96] = {
         16,    17,    18,    19,    10,    11,    20,    21,    12,    13,    14,    15,   208,   209,   210,   211,
         48,    49,    50,    51,    42,    43,    52,    53,    44,    45,    46,    47,   202,   203,   212,   213,
         80,    81,    82,    83,    74,    75,    84,    85,    76,    77,    78,    79,   204,   205,   206,   207,
        112,   113,   114,   115,   106,   107,   116,   117,   108,   109,   110,   111,   240,   241,   242,   243,
        144,   145,   146,   147,   138,   139,   148,   149,   140,   141,   142,   143,   234,   235,   244,   245,
        176,   177,   178,   179,   170,   171,   180,   181,   172,   173,   174,   175,   236,   237,   238,   239
    };

    // Induction:
    // min_channel is 0 ie, offline 7680
    // Map from register position to offline offset relative to minimum induction channel in register array:
    const int induction_offlines[160] = {
         974,   976,   978,   229,   973,   971,   224,   226,   227,   225,   970,   972,   228,   979,   977,   975,
         964,   966,   968,   239,   963,   961,   234,   236,   237,   235,   960,   962,   238,   969,   967,   965,
         984,   986,   988,   219,   983,   981,   214,   216,   217,   215,   980,   982,   218,   989,   987,   985,
         994,   996,   998,   209,   993,   991,   204,   206,   207,   205,   990,   992,   208,   999,   997,   995,
        1174,  1176,  1178,    29,  1173,  1171,    24,    26,    27,    25,  1170,  1172,    28,  1179,  1177,  1175,
        1164,  1166,  1168,    39,  1163,  1161,    34,    36,    37,    35,  1160,  1162,    38,  1169,  1167,  1165,
        1184,  1186,  1188,    19,  1183,  1181,    14,    16,    17,    15,  1180,  1182,    18,  1189,  1187,  1185,
        1194,  1196,  1198,     9,  1193,  1191,     4,     6,     7,     5,  1190,  1192,     8,  1199,  1197,  1195,
         223,   221,   233,   231,   220,   222,   230,   232,   213,   211,   203,   201,   210,   212,   200,   202,
          23,    21,    33,    31,    20,    22,    30,    32,    13,    11,     3,     1,    10,    12,     0,     2
    };

    // Map from register position to online channel number within (crate,fiber,slot):
    const int induction_index_to_chan[160] = {
          0,     1,     2,     3,     8,     9,    26,    27,     4,     5,    22,    23,    28,    29,    30,    31,
         32,    33,    34,    35,    40,    41,    58,    59,    36,    37,    54,    55,    60,    61,    62,    63,
         64,    65,    66,    67,    72,    73,    90,    91,    68,    69,    86,    87,    92,    93,    94,    95,
         96,    97,    98,    99,   104,   105,   122,   123,   100,   101,   118,   119,   124,   125,   126,   127,
        128,   129,   130,   131,   136,   137,   154,   155,   132,   133,   150,   151,   156,   157,   158,   159,
        160,   161,   162,   163,   168,   169,   186,   187,   164,   165,   182,   183,   188,   189,   190,   191,
        192,   193,   194,   195,   200,   201,   218,   219,   196,   197,   214,   215,   220,   221,   222,   223,
        224,   225,   226,   227,   232,   233,   250,   251,   228,   229,   246,   247,   252,   253,   254,   255,
          6,     7,    38,    39,    24,    25,    56,    57,    70,    71,   102,   103,    88,    89,   120,   121,
        134,   135,   166,   167,   152,   153,   184,   185,   198,   199,   230,   231,   216,   217,   248,   249
    };

}

//==============================================================================
int collection_index_to_offline(int index)
{
    if(index<0 || index>95)  return -1;
    else                     return collection_offlines[index];
}

//==============================================================================
int induction_index_to_offline(int index)
{
    if(index<0 || index>159) return -1;
    else                     return induction_offlines[index];
}

//==============================================================================
int collection_index_to_channel(int index)
{
    if(index<0 || index>95)  return -1;
    else                     return collection_index_to_chan[index];
}

//==============================================================================
int induction_index_to_channel(int index)
{
    if(index<0 || index>159) return -1;
    else                     return induction_index_to_chan[index];
}

} // namespace swtpg

