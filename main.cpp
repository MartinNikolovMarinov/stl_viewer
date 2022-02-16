#include <string>
#include <vector>

#include "src/stl_viewer.h"

using namespace core;

void CalcNormal(f32 p1[3], f32 p2[3], f32 p3[3], f32 normal[3])
{
    f32 v1[3] = { p2[0] - p1[0], p2[1] - p1[1], p2[2] - p1[2] };
    f32 v2[3] = { p3[0] - p1[0], p3[1] - p1[1], p3[2] - p1[2] };
    normal[0] = v1[1] * v2[2] - v1[2] * v2[1];
    normal[1] = v1[2] * v2[0] - v1[0] * v2[2];
    normal[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

Optional<signed_ptr_size, OSErrCode> DirectWriteCharPtrToFile(OsFile _file, constptr char *_data, ptr_size _len)
{
    i32 writtenBytes;
    TryOrReturn(writtenBytes, OsWrite(_file, _data, _len));
    return Optional<signed_ptr_size, OSErrCode>(writtenBytes, null);
}

Optional<signed_ptr_size, OSErrCode> DirectWriteToFile(OsFile _file, constptr char *_data)
{
    return DirectWriteCharPtrToFile(_file, _data, CharPtrLen(_data));
}

Optional<i32> WriteSTLFileASCII(f32 _nodes[][3], i32 _nodesLen,
                                i32 _triangles[][3], i32 _trianglesLen,
                                constptr char *_fileName)
{
    auto openOpt = OsOpen(_fileName, (OpenFlag)((u32)OpenFlag::READ_WRITE | (u32)OpenFlag::TRUNC));
    if (openOpt.err != null) {
        return Optional<i32>((i32)ErrCodes::ERROR, "failed to open file");
    }

    OsFile file = openOpt.val;
    Optional<signed_ptr_size, OSErrCode> writeOpt;

    // BEGIN solid:
    writeOpt = DirectWriteToFile(file, "solid \n");
    if (writeOpt.err != null) {
        return Optional<i32>((i32)ErrCodes::ERROR, "failed to write to file");
    }

    std::string lineBuff;
    for (i32 i = 0; i < _trianglesLen; i++) {
        lineBuff.clear();

        f32 normal[3];
        CalcNormal(_nodes[_triangles[i][0]],
                   _nodes[_triangles[i][1]],
                   _nodes[_triangles[i][2]],
                   normal);

        // Write line to buffer:
        lineBuff.append(stlview::string_format("facet normal %f %f %f\n",
                                               normal[0],
                                               normal[1],
                                               normal[2]));
        lineBuff.append("\touter loop\n");
        for (i32 j = 0; j < 3; j++) {
            lineBuff.append(stlview::string_format("\t\tvertex %f %f %f\n",
                            _nodes[_triangles[i][j]][0],
                            _nodes[_triangles[i][j]][1],
                            _nodes[_triangles[i][j]][2]));
        }
        lineBuff.append("\tendloop\n");

        // Write line to file:
        writeOpt = DirectWriteCharPtrToFile(file, lineBuff.c_str(), lineBuff.size());
        if (writeOpt.err != null) {
            return Optional<i32>((i32)ErrCodes::ERROR, "failed to write to file");
        }
    }

    // END solid:
    writeOpt = DirectWriteToFile(file, "endsolid");
    if (writeOpt.err != null) {
        return Optional<i32>((i32)ErrCodes::ERROR, "failed to write to file");
    }

    return Optional<i32>((i32)ErrCodes::OK, null);
}

Optional<i32> WriteSTLFileBinary(f32 _nodes[][3], i32 _nodesLen,
                                 i32 _triangles[][3], i32 _trianglesLen,
                                 constptr char *_fileName)
{
    auto openOpt = OsOpen(_fileName, (OpenFlag)((u32)OpenFlag::READ_WRITE | (u32)OpenFlag::TRUNC));
    if (openOpt.err != null) {
        return Optional<i32>((i32)ErrCodes::ERROR, "failed to open file");
    }

    OsFile file = openOpt.val;
    Optional<signed_ptr_size, OSErrCode> writeOpt;
    const i32 HEADER_SIZE = 80;
    const i32 ATTRIBUTE_BYTE_COUNT_SIZE = 2;
    u8 header[HEADER_SIZE] = {};
    u8 attributeByteCount[ATTRIBUTE_BYTE_COUNT_SIZE] = {};

    // Write Header:
    writeOpt = OsWrite(file, header, sizeof(u8) * HEADER_SIZE);
    if (writeOpt.err != null) {
        return Optional<i32>((i32)ErrCodes::ERROR, "failed to write to file");
    }
    // Write number of triangles:
    writeOpt = OsWrite(file, &_trianglesLen, sizeof(i32));
    if (writeOpt.err != null) {
        return Optional<i32>((i32)ErrCodes::ERROR, "failed to write to file");
    }

    // Write Triangles:
    std::vector<u8> buff;
    for (i32 i = 0; i < _trianglesLen; i++) {
        buff.clear();

        f32 normal[3];
        CalcNormal(_nodes[_triangles[i][0]],
                   _nodes[_triangles[i][1]],
                   _nodes[_triangles[i][2]],
                   normal);

        // FIXME: move this nonsense to a function... mind the endianness...
        u8 f32Bytes[4];
        FloatToBinaryF32(f32Bytes, normal[0]);
        buff.push_back(f32Bytes[0]);
        buff.push_back(f32Bytes[1]);
        buff.push_back(f32Bytes[2]);
        buff.push_back(f32Bytes[3]);
        FloatToBinaryF32(f32Bytes, normal[1]);
        buff.push_back(f32Bytes[0]);
        buff.push_back(f32Bytes[1]);
        buff.push_back(f32Bytes[2]);
        buff.push_back(f32Bytes[3]);
        FloatToBinaryF32(f32Bytes, normal[2]);
        buff.push_back(f32Bytes[0]);
        buff.push_back(f32Bytes[1]);
        buff.push_back(f32Bytes[2]);
        buff.push_back(f32Bytes[3]);

        for (i32 j = 0; j < 3; j++) {
            FloatToBinaryF32(f32Bytes, _nodes[_triangles[i][j]][0]);
            buff.push_back(f32Bytes[0]);
            buff.push_back(f32Bytes[1]);
            buff.push_back(f32Bytes[2]);
            buff.push_back(f32Bytes[3]);
            FloatToBinaryF32(f32Bytes, _nodes[_triangles[i][j]][1]);
            buff.push_back(f32Bytes[0]);
            buff.push_back(f32Bytes[1]);
            buff.push_back(f32Bytes[2]);
            buff.push_back(f32Bytes[3]);
            FloatToBinaryF32(f32Bytes, _nodes[_triangles[i][j]][2]);
            buff.push_back(f32Bytes[0]);
            buff.push_back(f32Bytes[1]);
            buff.push_back(f32Bytes[2]);
            buff.push_back(f32Bytes[3]);
        }

        // Add attributes count
        buff.push_back(attributeByteCount[0]);
        buff.push_back(attributeByteCount[1]);

        // Write line to file:
        writeOpt = OsWrite(file, buff.data(), buff.size());
        if (writeOpt.err != null) {
            return Optional<i32>((i32)ErrCodes::ERROR, "failed to write to file");
        }
    }

    return Optional<i32>((i32)ErrCodes::OK, null);
}

i32 main(i32 _argc, constptr char *_argv[])
{
    f32 nodes[9][3] = {
        {0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
        {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1},
        {0.5, 0.5, 1.5},
    };
    int triangles[14][3] = {
        {0, 1, 5}, {0, 5, 4}, {1, 2, 6}, {1, 6, 5}, {2, 3, 7}, {2, 7, 6},
        {3, 0, 4}, {3, 4, 7}, {4, 5, 8}, {5, 6, 8}, {6, 7, 8}, {7, 4, 8},
        {0, 3, 2}, {0, 2, 1}
    };
    char fileName[] = "house.stl";
    char fileNameBin[] = "house_bin.stl";

    // TryOrFailIgnoreOut(WriteSTLFileASCII(nodes, 9, triangles, 14, fileName));
    TryOrFailIgnoreOut(WriteSTLFileBinary(nodes, 9, triangles, 14, fileNameBin));

    return 0;
}
