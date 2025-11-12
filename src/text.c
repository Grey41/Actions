#define STB_TRUETYPE_IMPLEMENTATION
#define HASH 2654435761
#define ATLAS 512
#define GLYPH 32
#include "main.h"

// static Glyph *get(Text *self, wchar_t key) {
//     size_t index = (key * HASH) & (self -> src -> capacity - 1);

//     while (self -> src -> chars[index].code) {
//         if (key == self -> src -> chars[index].code)
//             return self -> src -> chars[index].value;

//         index = index == self -> src -> capacity - 1 ? 0 : index + 1;
//     }

//     return NULL;
// }

// static void entry(Entry *entries, size_t capacity, wchar_t key, Glyph *value) {
//     size_t index = (key * HASH) & (capacity - 1);

//     while (entries[index].code)
//         index = index == capacity - 1 ? 0 : index + 1;

//     entries[index].code = key;
//     entries[index].value = value;
// }

// static int set(Text *self, wchar_t key, Glyph *value) {
//     if (self -> src -> length > self -> src -> capacity / 2) {
//         size_t size = self -> src -> capacity * 2;
//         Entry *entries = calloc(size, sizeof(Entry));

//         if (!entries)
//             return PyErr_NoMemory(), -1;

//         for (size_t i = 0; i < self -> src -> capacity; i ++) {
//             Entry item = self -> src -> chars[i];

//             if (item.code)
//                 entry(entries, size, item.code, item.value);
//         }

//         free(self -> src -> chars);
//         self -> src -> chars = entries;
//         self -> src -> capacity = size;
//     }

//     return entry(self -> src -> chars, self -> src -> capacity, key, value), 0;
// }

static int create(Text *self) {
    float x = 0, y = 0;
    double offset = self -> src -> descend / 2 + self -> src -> ascend / 2;

    size_t size = self -> len * 24 * sizeof(GLfloat);
    GLfloat *data = malloc(size);

    if (!data)
        return PyErr_NoMemory(), -1;

    for (size_t i = 0; i < self -> len; i ++) {
        size_t j = i * 24;

        stbtt_aligned_quad quad;
        stbtt_GetPackedQuad(self -> src -> chars, ATLAS, ATLAS, self -> content[i] - 32, &x, &y, &quad, 1);

        data[j] = data[j + 12] = data[j + 20] = quad.x0 / GLYPH;
        data[j + 1] = data[j + 5] = data[j + 13] = quad.y0 / -GLYPH - offset;
        data[j + 2] = data[j + 14] = data[j + 22] = quad.s0;
        data[j + 3] = data[j + 7] = data[j + 15] = quad.t0;

        data[j + 4] = data[j + 8] = data[j + 16] = quad.x1 / GLYPH;
        data[j + 9] = data[j + 17] = data[j + 21] = quad.y1 / -GLYPH - offset;
        data[j + 6] = data[j + 10] = data[j + 18] = quad.s1;
        data[j + 11] = data[j + 19] = data[j + 23] = quad.t1;
    }

    self -> width = x / GLYPH;

    printf("x %f \n", x);

    glBindVertexArray(self -> vao);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);

    return free(data), 0;
}

static int load(Text *self, const char *name) {
    for (Font *this = fonts; this; this = this -> next)
        if (!strcmp(this -> name, name))
            return self -> src = this, 0;

    FILE *file = fopen(name, "rb");

    if (file) {
        if (!fseek(file, 0, SEEK_END)) {
            const long size = ftell(file);

            if (size >= 0 && !fseek(file, 0, SEEK_SET)) {
                unsigned char *buffer = malloc(size);

                if (!buffer) {
                    PyErr_NoMemory();
                    return fclose(file), -1;
                }

                if (fread(buffer, 1, size, file) >= size || !ferror(file)) {
                    fclose(file);

                    unsigned char atlas[ATLAS * ATLAS];
                    stbtt_pack_context ctx;

                    Font *font = malloc(sizeof(Font));

                    if (font) {
                        font -> next = fonts;
                        fonts = self -> src = font;

                        if (stbtt_InitFont(&font -> info, buffer, 0)) {
                            if ((font -> name = strdup(name)) && stbtt_PackBegin(&ctx, atlas, ATLAS, ATLAS, 0, 1, NULL)) {
                                if (stbtt_PackFontRange(&ctx, buffer, 0, GLYPH, 32, 95, font -> chars)) {
                                    stbtt_PackEnd(&ctx);
                                    free(buffer);

                                    glGenTextures(1, &font -> atlas);
                                    glBindTexture(GL_TEXTURE_2D, font -> atlas);
                                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, ATLAS, ATLAS, 0, GL_RED, GL_UNSIGNED_BYTE, atlas);
                                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                                    return 0;
                                }

                                stbtt_PackEnd(&ctx);
                            }

                            PyErr_NoMemory();
                        }

                        else PyErr_SetString(PyExc_OSError, "Failed to load font");
                    }

                    else PyErr_NoMemory();
                    return free(buffer), -1;
                }

                free(buffer);
            }
        }

        fclose(file);
    }

    return PyErr_SetFromErrno(PyExc_OSError), -1;
}

// static void delete(Text *self) {
//     for (FT_Long i = 0; self -> src && i < self -> src -> face -> num_glyphs; i ++)
//         glDeleteTextures(1, &self -> chars[i].src);
// }

static Text *text_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    Text *self = (Text *) type -> tp_alloc(type, 0);

    if (self) {
        glGenVertexArrays(1, &self -> vao);
        glGenBuffers(1, &self -> vbo);

        glBindVertexArray(self -> vao);
        glBindBuffer(GL_ARRAY_BUFFER, self -> vbo);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4, 0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4, (void *) (sizeof(GLfloat) * 2));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
    }

    return self;
}

static int text_init(Text *self, PyObject *args, PyObject *kwds) {
    static char *kwlist[] = {"content", "x", "y", "angle", "color", "font", NULL};

    PyObject *color = NULL;
    PyObject *src = PyObject_GetAttrString(program, "DEFAULT");

    INIT(!src)
    BaseType.tp_init((PyObject *) self, NULL, NULL);
    self -> size = 100;

    const char *file = PyUnicode_AsUTF8(src);
    const char *content = NULL;

    if (!file || !PyArg_ParseTupleAndKeywords(
        args, kwds, "|s#dddOs:Text", kwlist,
        &content, &self -> len,
        &self -> base.base.pos.x, &self -> base.base.pos.y,
        &self -> base.base.angle, &color,
        &file) || load(self, file)) return Py_DECREF(src), -1;

    Py_DECREF(src);

    if (!content) {
        content = "Text";
        self -> len = 4;
    }

    self -> content = strndup(content, self -> len);
    return self -> content ? vector_set(color, (double *) &self -> base.base.color, 4) ? -1 : create(self) : (PyErr_NoMemory(), -1);
}

static PyObject *text_draw(Text *self, PyObject *args) {
    // double pen = self -> base.base.anchor.x - self -> vect.x / 2;

    // const double sx = self -> base.base.scale.x + self -> base.size.x / self -> vect.x - 1;
    // const double sy = self -> base.base.scale.y + self -> base.size.y / self -> vect.y - 1;
    // const double sine = sin(self -> base.base.angle * M_PI / 180);
    // const double cosine = cos(self -> base.base.angle * M_PI / 180);

    // glUseProgram(shader.image.src);
    // glUniform4f(shader.image.color, self -> base.base.color.x, self -> base.base.color.y, self -> base.base.color.z, self -> base.base.color.w);
    // glBindVertexArray(shader.vao);
    // glBindTexture(GL_TEXTURE_2D, self -> test);

    // printf("%f %f %f\n", self -> base.base.color.x, self -> base.base.color.y, self -> base.base.color.z);

    glBindTexture(GL_TEXTURE_2D, self -> src -> atlas);
    glUseProgram(shader.text.src);
    // base_matrix(&self -> base.base, shader.text.obj, shader.text.color, self -> size, self -> size);

    GLfloat matrix[] = {
        self -> size * self -> base.base.scale.x, 0, 0,
        0, self -> size * self -> base.base.scale.y, 0,
        self -> base.base.pos.x - self -> width * self -> size / 2,
        self -> base.base.pos.y, 1
    };

    glUniformMatrix3fv(shader.text.obj, 1, GL_FALSE, matrix);
    glUniform4f(shader.text.color, self -> base.base.color.x, self -> base.base.color.y, self -> base.base.color.z, self -> base.base.color.w);

    glBindVertexArray(self -> vao);
    glDrawArrays(GL_TRIANGLES, 0, self -> len * 6);

    // for (size_t i = 0; self -> content[i]; i ++) {
    //     stbtt_GetPackedQuad(localState.packedChars,              // Array of stbtt_packedchar
    //   fontAtlasWidth,                      // Width of the font atlas texture
    //   fontAtlasHeight,                     // Height of the font atlas texture
    //   i,                                   // Index of the glyph
    //   &unusedX, &unusedY,                  // current position of the glyph in screen pixel coordinates, (not required as we have a different corrdinate system)
    //   &alignedQuads[i],                    // stbtt_alligned_quad struct. (this struct mainly consists of the texture coordinates)
    //   0                                    // Allign X and Y position to a integer (doesn't matter because we are not using 'unusedX' and 'unusedY')
    //   );

    //     Glyph glyph = self -> chars[FT_Get_Char_Index(self -> src -> face, self -> content[i])];
    //     if (!i) pen -= glyph.pos.x;

    //     const double ax = pen + glyph.pos.x + glyph.size.x / 2;
    //     const double ay = self -> base.base.anchor.y + glyph.pos.y - (glyph.size.y + self -> vect.y) / 2 - self -> descend;

    //     GLfloat matrix[] = {
    //         glyph.size.x * sx * cosine, glyph.size.x * sx * sine, 0,
    //         glyph.size.y * sy * -sine, glyph.size.y * sy * cosine, 0,
    //         ax * sx * cosine + ay * sy * -sine + self -> base.base.pos.x,
    //         ax * sx * sine + ay * sy * cosine + self -> base.base.pos.y, 1
    //     };

    //     glBindTexture(GL_TEXTURE_2D, glyph.src);
    //     glUniformMatrix3fv(shader.text.obj, 1, GL_FALSE, matrix);
    //     pen += glyph.advance;

    //     glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    // }

    // glBindVertexArray(0);
    Py_RETURN_NONE;
}

static void text_dealloc(Text *self) {
    printf("OOO\n");
    // delete(self);
    printf("aaa\n");
    // free(self -> chars);
    free(self -> content);
}

static PyMethodDef text_methods[] = {
    {"draw", (PyCFunction) text_draw, METH_NOARGS, "Draw the text on the screen"},
    {NULL}
};

PyTypeObject TextType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "Text",
    .tp_doc = "Render text on the screen",
    .tp_basicsize = sizeof(Text),
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_base = &RectType,
    .tp_new = (newfunc) text_new,
    .tp_init = (initproc) text_init,
    .tp_dealloc = (destructor) text_dealloc,
    .tp_methods = text_methods
};