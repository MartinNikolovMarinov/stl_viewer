#include <string>

#include "src/stl_viewer.h"

using namespace core;

void GetNormal(f32 p1[3], f32 p2[3], f32 p3[3], f32 normal[3])
{
    f32 v1[3] = { p2[0] - p1[0], p2[1] - p1[1], p2[2] - p1[2] };
    f32 v2[3] = { p3[0] - p1[0], p3[1] - p1[1], p3[2] - p1[2] };
    normal[0] = v1[1] * v2[2] - v1[2] * v2[1];
    normal[1] = v1[2] * v2[0] - v1[0] * v2[2];
    normal[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

Optional<signed_ptr_size, OSErrCode> DirectWriteToFile(OsFile _file, constptr char *_data, ptr_size _len)
{
    i32 writtenBytes;
    TryOrReturn(writtenBytes, OsWrite(_file, _data, _len));
    return Optional<signed_ptr_size, OSErrCode>(writtenBytes, null);
}

Optional<signed_ptr_size, OSErrCode> DirectWriteToFile(OsFile _file, constptr char *_data)
{
    return DirectWriteToFile(_file, _data, CharPtrLen(_data));
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
        GetNormal(_nodes[_triangles[i][0]],
                  _nodes[_triangles[i][1]],
                  _nodes[_triangles[i][2]],
                  normal);

        // Generate line to line buffer:
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
        writeOpt = DirectWriteToFile(file, lineBuff.c_str(), lineBuff.size());
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

    TryOrFailIgnoreOut(WriteSTLFileASCII(nodes, 9, triangles, 14, fileName));

    return 0;
}
