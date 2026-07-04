#include <stddef.h>

typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef signed char GLbyte;
typedef short GLshort;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned int GLuint;
typedef float GLfloat;
typedef float GLclampf;
typedef void GLvoid;
typedef ptrdiff_t GLintptr;
typedef ptrdiff_t GLsizeiptr;
typedef char GLchar;

extern "C" {

void glActiveTexture(GLenum texture) {}
void glAttachShader(GLuint program, GLuint shader) {}
void glBindAttribLocation(GLuint program, GLuint index, const GLchar *name) {}
void glBindBuffer(GLenum target, GLuint buffer) {}
void glBindFramebuffer(GLenum target, GLuint framebuffer) {}
void glBindTexture(GLenum target, GLuint texture) {}
void glBlendEquation(GLenum mode) {}
void glBlendFunc(GLenum sfactor, GLenum dfactor) {}
void glBufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage) {}
void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void *data) {}
GLenum glCheckFramebufferStatus(GLenum target) { return 0; }
void glClear(GLbitfield mask) {}
void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {}
void glClearStencil(GLint s) {}
void glCompileShader(GLuint shader) {}
void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data) {}
GLuint glCreateProgram(void) { return 0; }
GLuint glCreateShader(GLenum type) { return 0; }
void glDeleteBuffers(GLsizei n, const GLuint *buffers) {}
void glDeleteFramebuffers(GLsizei n, const GLuint *framebuffers) {}
void glDeleteProgram(GLuint program) {}
void glDeleteShader(GLuint shader) {}
void glDeleteTextures(GLsizei n, const GLuint *textures) {}
void glDepthMask(GLboolean flag) {}
void glDisable(GLenum cap) {}
void glDisableVertexAttribArray(GLuint index) {}
void glDrawArrays(GLenum mode, GLint first, GLsizei count) {}
void glDrawElements(GLenum mode, GLsizei count, GLenum type, const void *indices) {}
void glEnable(GLenum cap) {}
void glEnableVertexAttribArray(GLuint index) {}
void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {}
void glGenBuffers(GLsizei n, GLuint *buffers) {}
void glGenerateMipmap(GLenum target) {}
void glGenFramebuffers(GLsizei n, GLuint *framebuffers) {}
void glGenTextures(GLsizei n, GLuint *textures) {}
GLenum glGetError(void) { return 0; }
void glGetIntegerv(GLenum pname, GLint *data) {}
void glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {}
void glGetProgramiv(GLuint program, GLenum pname, GLint *params) {}
void glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {}
void glGetShaderiv(GLuint shader, GLenum pname, GLint *params) {}
const GLubyte *glGetString(GLenum name) { return (const GLubyte *)""; }
GLint glGetUniformLocation(GLuint program, const GLchar *name) { return 0; }
void glHint(GLenum target, GLenum mode) {}
void glLinkProgram(GLuint program) {}
void glPixelStorei(GLenum pname, GLint param) {}
void glScissor(GLint x, GLint y, GLsizei width, GLsizei height) {}
void glShaderSource(GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length) {}
void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels) {}
void glTexParameterf(GLenum target, GLenum pname, GLfloat param) {}
void glTexParameteri(GLenum target, GLenum pname, GLint param) {}
void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels) {}
void glUniform1f(GLint location, GLfloat v0) {}
void glUniform1fv(GLint location, GLsizei count, const GLfloat *value) {}
void glUniform1i(GLint location, GLint v0) {}
void glUniform2f(GLint location, GLfloat v0, GLfloat v1) {}
void glUniform2fv(GLint location, GLsizei count, const GLfloat *value) {}
void glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {}
void glUniform3fv(GLint location, GLsizei count, const GLfloat *value) {}
void glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {}
void glUniform4fv(GLint location, GLsizei count, const GLfloat *value) {}
void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {}
void glUseProgram(GLuint program) {}
void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer) {}
void glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {}

}
