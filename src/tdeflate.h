// This file is part of Notepad++ plugin MIME Tools project
// Copyright (C)2026 ExSlam contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.

#pragma once

#include <cstddef>
#include <vector>

// Compresses input as an RFC 1951 raw DEFLATE stream (no zlib/gzip wrapper).
// The encoder uses fixed Huffman blocks with LZ77 matching and falls back to
// stored blocks if that would be smaller. Returns false on allocation failure.
bool tdefl_compress_raw(std::vector<unsigned char>& output,
                        const unsigned char* input,
                        std::size_t inputLength);
