/*
 * Copyright 2010 Google, Inc.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2,
 * as published by the Free Software Foundation.
 *
 * In addition to the permissions in the GNU General Public License,
 * the authors give you unlimited permission to link the compiled
 * version of this file into combinations with other programs,
 * and to distribute those combinations without any restriction
 * coming from the use of this file.  (The General Public License
 * restrictions do apply in other respects; for example, they cover
 * modification of the file, and distribution when not linked into
 * a combined executable.)
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <Python.h>
#include <git/common.h>
#include <git/repository.h>
#include <git/commit.h>
#include <git/odb.h>

typedef struct {
    PyObject_HEAD
    git_repository *repo;
} Repository;

typedef struct {
    PyObject_HEAD
    Repository *repo;
    git_object *obj;
    int own_obj:1;
} Object;

static PyTypeObject RepositoryType, ObjectType;

static int
Repository_init(Repository *self, PyObject *args, PyObject *kwds) {
    char *path;

    if (kwds) {
        PyErr_SetString(PyExc_TypeError,
                        "Repository takes no keyword arugments");
        return -1;
    }

    if (!PyArg_ParseTuple(args, "s", &path))
        return -1;

    self->repo = git_repository_open(path);
    if (!self->repo) {
        PyErr_Format(PyExc_RuntimeError, "Failed to open repo directory at %s",
                     path);
        return -1;
    }

    return 0;
}

static void
Repository_dealloc(Repository *self) {
    if (self->repo)
        git_repository_free(self->repo);
    self->ob_type->tp_free((PyObject*)self);
}

static int
Repository_contains(Repository *self, PyObject *value) {
    char *hex;
    git_oid oid;

    hex = PyString_AsString(value);
    if (!hex)
        return -1;
    if (git_oid_mkstr(&oid, hex) < 0) {
        PyErr_Format(PyExc_ValueError, "Invalid hex SHA \"%s\"", hex);
        return -1;
    }
    return git_odb_exists(git_repository_database(self->repo), &oid);
}

static Object *wrap_object(git_object *obj, Repository *repo) {
    Object *py_obj = NULL;
    switch (git_object_type(obj)) {
        case GIT_OBJ_COMMIT:
            py_obj = (Object*)ObjectType.tp_alloc(&ObjectType, 0);
            break;
        case GIT_OBJ_TREE:
            py_obj = (Object*)ObjectType.tp_alloc(&ObjectType, 0);
            break;
        case GIT_OBJ_BLOB:
            py_obj = (Object*)ObjectType.tp_alloc(&ObjectType, 0);
            break;
        case GIT_OBJ_TAG:
            py_obj = (Object*)ObjectType.tp_alloc(&ObjectType, 0);
            break;
        default:
            assert(0);
    }
    if (!py_obj)
        return NULL;

    py_obj->obj = obj;
    py_obj->repo = repo;
    Py_INCREF(repo);
    return py_obj;
}

static PyObject *
Repository_getitem(Repository *self, PyObject *value) {
    char *hex;
    git_oid oid;
    git_object *obj;
    Object *py_obj;

    hex = PyString_AsString(value);
    if (!hex)
        return NULL;
    if (git_oid_mkstr(&oid, hex) < 0) {
        PyErr_Format(PyExc_ValueError, "Invalid hex SHA \"%s\"", hex);
        return NULL;
    }

    obj = git_repository_lookup(self->repo, &oid, GIT_OBJ_ANY);
    if (!obj) {
        PyErr_Format(PyExc_RuntimeError, "Failed to look up hex SHA \"%s\"",
                     hex);
        return NULL;
    }

    py_obj = wrap_object(obj, self);
    if (!py_obj)
        return NULL;
    py_obj->own_obj = 0;
    return (PyObject*)py_obj;
}

static PySequenceMethods Repository_as_sequence = {
    0,                               /* sq_length */
    0,                               /* sq_concat */
    0,                               /* sq_repeat */
    0,                               /* sq_item */
    0,                               /* sq_slice */
    0,                               /* sq_ass_item */
    0,                               /* sq_ass_slice */
    (objobjproc)Repository_contains, /* sq_contains */
};

static PyMappingMethods Repository_as_mapping = {
    0,                               /* mp_length */
    (binaryfunc)Repository_getitem,  /* mp_subscript */
    0,                               /* mp_ass_subscript */
};

static PyTypeObject RepositoryType = {
    PyObject_HEAD_INIT(NULL)
    0,                                         /* ob_size */
    "pygit2.Repository",                       /* tp_name */
    sizeof(Repository),                        /* tp_basicsize */
    0,                                         /* tp_itemsize */
    (destructor)Repository_dealloc,            /* tp_dealloc */
    0,                                         /* tp_print */
    0,                                         /* tp_getattr */
    0,                                         /* tp_setattr */
    0,                                         /* tp_compare */
    0,                                         /* tp_repr */
    0,                                         /* tp_as_number */
    &Repository_as_sequence,                   /* tp_as_sequence */
    &Repository_as_mapping,                    /* tp_as_mapping */
    0,                                         /* tp_hash  */
    0,                                         /* tp_call */
    0,                                         /* tp_str */
    0,                                         /* tp_getattro */
    0,                                         /* tp_setattro */
    0,                                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  /* tp_flags */
    "Git repository",                          /* tp_doc */
    0,                                         /* tp_traverse */
    0,                                         /* tp_clear */
    0,                                         /* tp_richcompare */
    0,                                         /* tp_weaklistoffset */
    0,                                         /* tp_iter */
    0,                                         /* tp_iternext */
    0,                                         /* tp_methods */
    0,                                         /* tp_members */
    0,                                         /* tp_getset */
    0,                                         /* tp_base */
    0,                                         /* tp_dict */
    0,                                         /* tp_descr_get */
    0,                                         /* tp_descr_set */
    0,                                         /* tp_dictoffset */
    (initproc)Repository_init,                 /* tp_init */
    0,                                         /* tp_alloc */
    0,                                         /* tp_new */
};

static void
Object_dealloc(Object* self)
{
    if (self->own_obj)
        git_object_free(self->obj);
    Py_DECREF(self->repo);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Object_get_type(Object *self, void *closure) {
    return PyInt_FromLong(git_object_type(self->obj));
}

static PyObject *
Object_get_sha(Object *self, void *closure) {
    char hex[GIT_OID_HEXSZ];
    git_oid_fmt(hex, git_object_id(self->obj));
    return PyString_FromStringAndSize(hex, GIT_OID_HEXSZ);
}

static PyObject *
Object_read_raw(Object *self) {
    git_odb *db;
    git_rawobj raw;
    PyObject *result;

    db = git_repository_database(self->repo->repo);
    if (git_odb_read(&raw, db, git_object_id(self->obj)) < 0) {
        PyErr_SetString(PyExc_RuntimeError, "Missing object");
        goto error;
    }

    result = PyString_FromStringAndSize(raw.data, raw.len);
    if (!result)
        goto error;
    free(raw.data);
    return result;

error:
    free(raw.data);
    return NULL;
}

static PyGetSetDef Object_getseters[] = {
    {"type", (getter)Object_get_type, NULL, "type number", NULL},
    {"sha", (getter)Object_get_sha, NULL, "hex SHA", NULL},
    {NULL}
};

static PyMethodDef Object_methods[] = {
    {"read_raw", (PyCFunction)Object_read_raw, METH_NOARGS,
     "Read the raw contents of the object from the repo."},
    {NULL}
};

static PyTypeObject ObjectType = {
    PyObject_HEAD_INIT(NULL)
    0,                                         /*ob_size*/
    "pygit2.Object",                           /*tp_name*/
    sizeof(Object),                            /*tp_basicsize*/
    0,                                         /*tp_itemsize*/
    (destructor)Object_dealloc,                /*tp_dealloc*/
    0,                                         /*tp_print*/
    0,                                         /*tp_getattr*/
    0,                                         /*tp_setattr*/
    0,                                         /*tp_compare*/
    0,                                         /*tp_repr*/
    0,                                         /*tp_as_number*/
    0,                                         /*tp_as_sequence*/
    0,                                         /*tp_as_mapping*/
    0,                                         /*tp_hash */
    0,                                         /*tp_call*/
    0,                                         /*tp_str*/
    0,                                         /*tp_getattro*/
    0,                                         /*tp_setattro*/
    0,                                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  /*tp_flags*/
    "Object objects",                          /* tp_doc */
    0,                                         /* tp_traverse */
    0,                                         /* tp_clear */
    0,                                         /* tp_richcompare */
    0,                                         /* tp_weaklistoffset */
    0,                                         /* tp_iter */
    0,                                         /* tp_iternext */
    Object_methods,                            /* tp_methods */
    0,                                         /* tp_members */
    Object_getseters,                          /* tp_getset */
    0,                                         /* tp_base */
    0,                                         /* tp_dict */
    0,                                         /* tp_descr_get */
    0,                                         /* tp_descr_set */
    0,                                         /* tp_dictoffset */
    0,                                         /* tp_init */
    0,                                         /* tp_alloc */
    0,                                         /* tp_new */
};

static PyMethodDef module_methods[] = {
    {NULL}
};

PyMODINIT_FUNC
initpygit2(void)
{
    PyObject* m;

    RepositoryType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&RepositoryType) < 0)
        return;
    /* Do not set ObjectType.tp_new, to prevent creating Objects directly. */
    if (PyType_Ready(&ObjectType) < 0)
        return;

    m = Py_InitModule3("pygit2", module_methods,
                       "Python bindings for libgit2.");

    if (m == NULL)
      return;

    Py_INCREF(&RepositoryType);
    PyModule_AddObject(m, "Repository", (PyObject *)&RepositoryType);

    Py_INCREF(&ObjectType);
    PyModule_AddObject(m, "Object", (PyObject *)&ObjectType);
}