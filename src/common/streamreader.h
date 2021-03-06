/*
 * Copyright (c) 2020 The reone project contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <cstdint>
#include <istream>
#include <memory>
#include <string>
#include <vector>

#include "types.h"

namespace reone {

class StreamReader {
public:
    StreamReader(const std::shared_ptr<std::istream> &stream, Endianess endianess = Endianess::Little);

    size_t tell();
    void seek(size_t pos);
    void ignore(int count);

    uint8_t getByte();
    uint16_t getUint16();
    uint32_t getUint32();
    uint64_t getUint64();
    int16_t getInt16();
    int32_t getInt32();
    int64_t getInt64();
    float getFloat();
    double getDouble();
    std::string getCString();
    std::string getString(int len);

    bool eof() const;

    template <class T>
    std::vector<T> getArray(int count);

private:
    std::shared_ptr<std::istream> _stream;
    Endianess _endianess;

    StreamReader(const StreamReader &) = delete;
    StreamReader &operator=(const StreamReader &) = delete;

    bool isSameEndianess() const;

    template <class T>
    void fixEndianess(T &val);
};

} // namespace reone
