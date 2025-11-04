#define STB_IMAGE_IMPLEMENTATION
#include "main.h"

static int load(Image *self, const char *name) {
    for (Texture *this = textures; this; this = this -> next)
        if (!strcmp(this -> name, name))
            return self -> src = this, 0;

    int width, height;
    stbi_uc *image = stbi_load(name, &width, &height, 0, STBI_rgb_alpha);

    if (!image)
        return PyErr_Format(PyExc_FileNotFoundError, "Failed to load image '%s'", name), -1;

    Texture *texture = malloc(sizeof(Texture));

    texture -> next = textures;
    textures = texture;

    glGenTextures(1, &textures -> src);
    glBindTexture(GL_TEXTURE_2D, textures -> src);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    textures -> width = width;
    textures -> height = height;

    textures -> name = strdup(name);
    self -> src = textures;

    return stbi_image_free(image), 0;
}

static PyObject *image_draw(Image *self, PyObject *args) {
    glBindTexture(GL_TEXTURE_2D, self -> src -> src);
    glUseProgram(shader.image);
    base_matrix(&self -> base.base, shader.i_obj, shader.i_color, self -> base.size.x, self -> base.size.y);

    glBindVertexArray(shader.vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    Py_RETURN_NONE;
}

static int image_init(Image *self, PyObject *args, PyObject *kwds) {
    static char *kwlist[] = {"name", "x", "y", "angle", "width", "height", "color", NULL};

    PyObject *color = NULL;
    PyObject *src = PyObject_GetAttrString(program, "MAN");

    INIT(!src)
    BaseType.tp_init((PyObject *) self, NULL, NULL);

    self -> base.size.x = 0;
    self -> base.size.y = 0;

    self -> base.base.color.x = 1;
    self -> base.base.color.y = 1;
    self -> base.base.color.z = 1;

    const char *name = PyUnicode_AsUTF8(src);

    return name && PyArg_ParseTupleAndKeywords(
        args, kwds, "|sdddddO:Image", kwlist,
        &name, &self -> base.base.pos.x,
        &self -> base.base.pos.y,
        &self -> base.base.angle,
        &self -> base.size.x,
        &self -> base.size.y,
        &color) && !load(self, name) ? (Py_DECREF(src),
        self -> base.size.x = self -> base.size.x || self -> src -> width,
        self -> base.size.y = self -> base.size.y || self -> src -> height,
        vector_set(color, (double *) &self -> base.base.color, 4)) : (Py_DECREF(src), -1);
}

static PyGetSetDef image_getset[] = {
    // {"name", (getter) image_get_name, (setter) image_set_name, "The path to the image file", NULL},
    {NULL}
};

static PyMethodDef image_methods[] = {
    {"draw", (PyCFunction) image_draw, METH_NOARGS, "Draw the image on the screen"},
    {NULL}
};

PyTypeObject ImageType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "Image",
    .tp_doc = "Render images on the screen",
    .tp_basicsize = sizeof(Image),
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_base = &RectType,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) image_init,
    .tp_methods = image_methods,
    .tp_getset = image_getset
};