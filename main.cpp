#include <string>
#include <vector>
#include <X11/Xlib.h>
#include <GL/glx.h>
#include <GL/glext.h>

#include "src/stl_viewer.h"

using namespace core;

const char *vertexShaderSource = "#version 330 core\n"
    "in vec3 aPos;\n"
    "uniform vec3 u1;\n"
    "uniform vec3 u2;\n"
    "uniform vec3 u3;\n"
    "uniform float scaling;\n"
    "uniform vec3 lookAt;\n"
    "void main()\n"
    "{\n"
    "	gl_Position = vec4((u1.x*(aPos.x-lookAt.x)+u1.y*(aPos.y-lookAt.y)+u1.z*(aPos.z-lookAt.z))*scaling,(u2.x*(aPos.x-lookAt.x)+u2.y*(aPos.y-lookAt.y)+u2.z*(aPos.z-lookAt.z))*scaling,0,1.0);\n"
    "}\0";

const char *fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);\n"
    "}\0";

typedef GLXContext (*glXCreateContextAttribsARBProc)
    (Display*, GLXFBConfig, GLXContext, bool8, const i32*);

void CalcNormal(f32 p1[3], f32 p2[3], f32 p3[3], f32 normal[3])
{
    f32 v1[3] = { p2[0] - p1[0], p2[1] - p1[1], p2[2] - p1[2] };
    f32 v2[3] = { p3[0] - p1[0], p3[1] - p1[1], p3[2] - p1[2] };
    normal[0] = v1[1] * v2[2] - v1[2] * v2[1];
    normal[1] = v1[2] * v2[0] - v1[0] * v2[2];
    normal[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

void NormalizeVector(f32 vect[3])
{
    f32 magintude = PowF32(vect[0]*vect[0] + vect[1]*vect[1] + vect[2]*vect[2], 0.5);
    vect[0] = vect[0] / magintude;
    vect[1] = vect[1] / magintude;
    vect[2] = vect[2] / magintude;
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

// TODO: error checking in Draw:
void Draw(i32 w, i32 h,
          f32 lookAt[3], f32 lookFrom[3],
          f32 _nodes[][3], i32 _nodesLen,
          i32 _triangles[][3], i32 _trianglesLen)
{
    // view frame axes
    f32 u3[3] = { lookAt[0]-lookFrom[0], lookAt[1]-lookFrom[1], lookAt[2]-lookFrom[2] };
    f32 u1[3] = { -u3[1], u3[0], 0 };
    f32 u2[3] = { -u3[2]*u3[0], -u3[2]*u3[1], (u3[0]*u3[0] + u3[1]*u3[1]) };

    // normalize view frame axes
    NormalizeVector(u1);
    NormalizeVector(u2);
    NormalizeVector(u3);

    Display *display = XOpenDisplay(0);
    Window window = XCreateSimpleWindow(display, DefaultRootWindow(display), 10, 10, w, h, 0, 0, 0);
    i32 attributes[] = {
        GLX_RENDER_TYPE, GLX_RGBA_BIT, GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_DOUBLEBUFFER, true, GLX_RED_SIZE, 1, GLX_GREEN_SIZE, 1,
        GLX_BLUE_SIZE, 1, None
    };
    i32 fbcId;
    GLXFBConfig *fbc = glXChooseFBConfig(display, DefaultScreen(display), attributes, &fbcId);

    glXCreateContextAttribsARBProc glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)glXGetProcAddress((const GLubyte*)"glXCreateContextAttribsARB");

    i32 ctx[] = {GLX_CONTEXT_MAJOR_VERSION_ARB, 4, GLX_CONTEXT_MINOR_VERSION_ARB, 2, None};
    GLXContext glxCtx = glXCreateContextAttribsARB(display, fbc[0], null, true, ctx);

    XMapWindow(display, window);
    XStoreName(display, window, "Viewer");
    glXMakeCurrent(display, window, glxCtx);

    u32 vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    char infoLog[512];
    i32 success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        PrintF("ERROR::SHADER::VERTEX::COMPILATION_FAILED %s\n",infoLog);
    }

    u32 fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);

    u32 shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    float vertices[3*_nodesLen];
    u32 indices[3* _trianglesLen];
    for (i32 i = 0; i< _nodesLen ; i++) {
        vertices[3*i] = _nodes[i][0];
        vertices[3*i+1] = _nodes[i][1];
        vertices[3*i+2] = _nodes[i][2];
    }
    for (i32 i = 0; i< _trianglesLen; i++) {
        indices[3*i] = _triangles[i][0];
        indices[3*i+1] = _triangles[i][1];
        indices[3*i+2] = _triangles[i][2];
    }

    u32 VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    GLint u1Loc = glGetUniformLocation(shaderProgram,"u1");
    GLint u2Loc = glGetUniformLocation(shaderProgram,"u2");
    GLint u3Loc = glGetUniformLocation(shaderProgram,"u3");
    GLint scalingLoc = glGetUniformLocation(shaderProgram,"scaling");
    GLint lookAtLoc = glGetUniformLocation(shaderProgram,"lookAt");

    glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);

    glUseProgram(shaderProgram);

    XEvent event;
    XSelectInput(display, window, (ButtonPressMask | ButtonReleaseMask | Button1MotionMask));

    f32 scaling = 0.5f;
    bool8 leftClick = false;
    i32 xStart = 0, yStart = 0;
    f32 thetaU1 = 0, thetaU2 = 0, mag = 0;
    f32 u1New[3], u2New[3], u3New[3];
    f32 u1Pend[3], u2Pend[3], u3Pend[3];

    u1New[0] = u1[0];
    u1New[1] = u1[1];
    u1New[2] = u1[2];

    u2New[0] = u2[0];
    u2New[1] = u2[1];
    u2New[2] = u2[2];

    u3New[0] = u3[0];
    u3New[1] = u3[1];
    u3New[2] = u3[2];

    while(true) {
        glClearColor(0.0f,0.0f,0.0f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUniform3f(u1Loc, u1New[0], u1New[1], u1New[2]);
        glUniform3f(u2Loc, u2New[0], u2New[1], u2New[2]);
        glUniform3f(u3Loc, u3New[0], u3New[1], u3New[2]);
        glUniform1f(scalingLoc, scaling);
        glUniform3f(lookAtLoc, lookAt[0], lookAt[1], lookAt[2]);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 3 * _trianglesLen, GL_UNSIGNED_INT, 0);
        glXSwapBuffers(display, window);

        // Mouse Events:
        XNextEvent(display, &event);

        if (event.xbutton.button == Button1 && event.xbutton.type == ButtonPress) {
            leftClick = true;
            xStart = event.xbutton.x;
            yStart = event.xbutton.y;
        }
        if (event.xbutton.button == Button1 && event.xbutton.type == ButtonRelease) {
            leftClick = false;

            u1[0] = u1New[0];
            u1[1] = u1New[1];
            u1[2] = u1New[2];

            u2[0] = u2New[0];
            u2[1] = u2New[1];
            u2[2] = u2New[2];

            u3[0] = u3New[0];
            u3[1] = u3New[1];
            u3[2] = u3New[2];
        }
        if (event.xbutton.button == Button4 && event.xbutton.type == ButtonPress) {
            scaling = scaling * 0.9f;
        }
        if (event.xbutton.button == Button5 && event.xbutton.type == ButtonPress) {
            scaling = scaling / 0.9f;
        }
        if (leftClick && event.xmotion.type == MotionNotify) {
            thetaU1 = (event.xmotion.y - yStart) / ((f32)h) * 15.0f / PI32;
            thetaU2 = (event.xmotion.x - xStart) / ((f32)w) * 15.0f / PI32;

            // NOTE: Uses Rodriguez rotation formula:
            //      https://en.wikipedia.org/wiki/Rodrigues%27_rotation_formula

            // Rotate u3 around u2 by thetaU2:
            mag = u2[0] * u3[0] + u2[1] * u3[1] + u2[2] * u3[2];
            u3New[0] = u3[0] * CosF32(thetaU2) + (u2[1] * u3[2] - u2[2] * u3[1]) * SinF32(thetaU2) + u2[0] * mag * (1 - CosF32(thetaU2));
            u3New[1] = u3[1] * CosF32(thetaU2) + (u2[2] * u3[0] - u2[0] * u3[2]) * SinF32(thetaU2) + u2[1] * mag * (1 - CosF32(thetaU2));
            u3New[2] = u3[2] * CosF32(thetaU2) + (u2[0] * u3[1] - u2[1] * u3[0]) * SinF32(thetaU2) + u2[2] * mag * (1 - CosF32(thetaU2));

            // Rotate u1 around u2 by thetaU2:
            mag = u2[0] * u1[0] + u2[1] * u1[1] + u2[2] * u1[2];
            u1New[0] = u1[0] * CosF32(thetaU2) + (u2[1] * u1[2] - u2[2] * u1[1]) * SinF32(thetaU2) + u2[0] * mag * (1 - CosF32(thetaU2));
            u1New[1] = u1[1] * CosF32(thetaU2) + (u2[2] * u1[0] - u2[0] * u1[2]) * SinF32(thetaU2) + u2[1] * mag * (1 - CosF32(thetaU2));
            u1New[2] = u1[2] * CosF32(thetaU2) + (u2[0] * u1[1] - u2[1] * u1[0]) * SinF32(thetaU2) + u2[2] * mag * (1 - CosF32(thetaU2));

            // Normalize:
            NormalizeVector(u1New);
            NormalizeVector(u3New);

            u1Pend[0] = u1New[0];
            u1Pend[1] = u1New[1];
            u1Pend[2] = u1New[2];
            u2Pend[0] = u2[0];
            u2Pend[1] = u2[1];
            u2Pend[2] = u2[2];
            u3Pend[0] = u3New[0];
            u3Pend[1] = u3New[1];
            u3Pend[2] = u3New[2];

            // Rotate u3 around u1 by thetaU1:
            mag = u1Pend[0] * u3Pend[0] + u1Pend[1] * u3Pend[1] + u1Pend[2] * u3Pend[2];
            u3New[0] = u3Pend[0] * CosF32(thetaU1) + (u1Pend[1] * u3Pend[2] - u1Pend[2] * u3Pend[1]) * SinF32(thetaU1) + u1Pend[0] * mag * (1 - CosF32(thetaU1));
            u3New[1] = u3Pend[1] * CosF32(thetaU1) + (u1Pend[2] * u3Pend[0] - u1Pend[0] * u3Pend[2]) * SinF32(thetaU1) + u1Pend[1] * mag * (1 - CosF32(thetaU1));
            u3New[2] = u3Pend[2] * CosF32(thetaU1) + (u1Pend[0] * u3Pend[1] - u1Pend[1] * u3Pend[0]) * SinF32(thetaU1) + u1Pend[2] * mag * (1 - CosF32(thetaU2));

            // Rotate u2 around u1 by thetaU1:
            mag = u1Pend[0] * u2Pend[0] + u1Pend[1] * u2Pend[1] + u1Pend[2] * u2Pend[2];
            u2New[0] = u2Pend[0] * CosF32(thetaU1) + (u1Pend[1] * u2Pend[2] - u1Pend[2] * u2Pend[1]) * SinF32(thetaU1) + u1Pend[0] * mag * (1 - CosF32(thetaU1));
            u2New[1] = u2Pend[1] * CosF32(thetaU1) + (u1Pend[2] * u2Pend[0] - u1Pend[0] * u2Pend[2]) * SinF32(thetaU1) + u1Pend[1] * mag * (1 - CosF32(thetaU1));
            u2New[2] = u2Pend[2] * CosF32(thetaU1) + (u1Pend[0] * u2Pend[1] - u1Pend[1] * u2Pend[0]) * SinF32(thetaU1) + u1Pend[2] * mag * (1 - CosF32(thetaU1));

            // Normalize:
            NormalizeVector(u2New);
            NormalizeVector(u3New);
        }
    };

    return;
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
    // TryOrFailIgnoreOut(WriteSTLFileBinary(nodes, 9, triangles, 14, fileNameBin));

    f32 lookAt[3] = {0.5, 0.5, 0.5};
    f32 lookFrom[3] = {0.5, -5, 0.5};
    Draw(800, 600, lookAt, lookFrom, nodes, 9, triangles, 14);

    return 0;
}
