/* bsdiff test
 */
#include <bsdiff.h>
#include <bspatch.h>
#include <fcntl.h>
#include <sdkconfig.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <unity.h>

static int _w(struct bsdiff_stream* stream, const void* buffer, int size)
{
    if (fwrite(buffer, size, 1, (FILE*)stream->opaque) != 1) {
        return -1;
    }
    return 0;
}

static int bsdiff_f(char* oldf, char* newf, char* patchf)
{
    int fd;
    uint8_t *old, *new;
    off_t oldsize, newsize;
    FILE* pf;
    struct bsdiff_stream stream;

    stream.malloc = malloc;
    stream.free = free;
    stream.write = _w;

    if (((fd = open(oldf, O_RDONLY, 0)) < 0) || ((oldsize = lseek(fd, 0, SEEK_END)) == -1)
        || ((old = malloc(oldsize + 1)) == NULL) || (lseek(fd, 0, SEEK_SET) != 0) || (read(fd, old, oldsize) != oldsize)
        || (close(fd) == -1)) {
        return -1;
    }

    if (((fd = open(newf, O_RDONLY, 0)) < 0) || ((newsize = lseek(fd, 0, SEEK_END)) == -1)
        || ((new = malloc(newsize + 1)) == NULL) || (lseek(fd, 0, SEEK_SET) != 0) || (read(fd, new, newsize) != newsize)
        || (close(fd) == -1)) {
        return -1;
    }

    if ((pf = fopen(patchf, "w")) == NULL) {
        return -1;
    }

    stream.opaque = pf;
    if (bsdiff(old, oldsize, new, newsize, &stream)) {
        return -1;
    }

    if (fclose(pf)) {
        return -1;
    }

    free(old);
    free(new);

    return newsize;
}

static int _r(const struct bspatch_stream* stream, void* buffer, int length)
{
    if (fread(buffer, 1, length, (FILE*)stream->opaque) != length) {
        return -1;
    }
    return 0;
}

static int _or(const struct bspatch_stream_i* stream, void* buffer, int pos, int length)
{
    uint8_t* old;
    old = (uint8_t*)stream->opaque;
    memcpy(buffer, old + pos, length);
    return 0;
}

struct NewCtx {
    uint8_t* new;
    int pos_write;
};

static int _nw(const struct bspatch_stream_n* stream, const void* buffer, int length)
{
    struct NewCtx* new;
    new = (struct NewCtx*)stream->opaque;
    memcpy(new->new + new->pos_write, buffer, length);
    new->pos_write += length;
    return 0;
}

static int bspatch_f(char* oldf, char* newf, size_t newfs, char* patchf)
{
    FILE* f;
    int fd;
    uint8_t *old, *new;
    int64_t oldsize;
    struct bspatch_stream stream;
    struct bspatch_stream_i oldstream;
    struct bspatch_stream_n newstream;
    struct stat sb;

    if ((f = fopen(patchf, "r")) == NULL) {
        return -1;
    }

    if (((fd = open(oldf, O_RDONLY, 0)) < 0) || ((oldsize = lseek(fd, 0, SEEK_END)) == -1)
        || ((old = malloc(oldsize + 1)) == NULL) || (lseek(fd, 0, SEEK_SET) != 0) || (read(fd, old, oldsize) != oldsize)
        || (fstat(fd, &sb)) || (close(fd) == -1)) {
        return -1;
    }

    if ((new = malloc(newfs + 1)) == NULL) {
        return -1;
    }

    stream.read = _r;
    oldstream.read = _or;
    newstream.write = _nw;
    stream.opaque = f;
    oldstream.opaque = old;
    struct NewCtx ctx = { .pos_write = 0, .new = new };
    newstream.opaque = &ctx;
    if (bspatch(&oldstream, oldsize, &newstream, newfs, &stream)) {
        return -1;
    }

    fclose(f);

    if (((fd = open(newf, O_CREAT | O_TRUNC | O_WRONLY, sb.st_mode)) < 0) || (write(fd, new, newfs) != newfs)
        || (close(fd) == -1)) {
        return -1;
    }

    free(new);
    free(old);

    return 0;
}

static int cmp(char* filename1, char* filename2)
{
    FILE* file1 = fopen(filename1, "rb");
    FILE* file2 = fopen(filename2, "rb");


    int ret = -1;

    int k, t;
    while (true) {
        t = getc(file1);
        k = getc(file2);
        if (t != k || k == EOF) {
            break;
        }
    }

    if (k == t) {
        ret = 0;
    }

    fclose(file1);
    fclose(file2);

    return ret;
}

void test_bsdiff_different_files(void)
{
    /* create the patch from two well known files */
    const int newsize = bsdiff_f("main/test_bsdiff.c", "main/CMakeLists.txt", "build/test_patch.bin");
    TEST_ASSERT_GREATER_THAN(1, newsize);
    /* apply the patch */
    const int bspatch_result = bspatch_f("main/test_bsdiff.c", "build/CMakeLists.txt", newsize, "build/test_patch.bin");
    TEST_ASSERT_EQUAL(0, bspatch_result);
    /* verify the new file is the same as the original */
    const int cmp_result = cmp("main/CMakeLists.txt", "build/CMakeLists.txt");
    TEST_ASSERT_EQUAL(0, cmp_result);
}

void test_bsdiff_same_file(void)
{
    /* create the patch from two well known files */
    const int newsize = bsdiff_f("main/test_bsdiff.c", "main/test_bsdiff.c", "build/test_patch.bin");
    TEST_ASSERT_GREATER_THAN(1, newsize);
    /* apply the patch */
    const int bspatch_result = bspatch_f("main/test_bsdiff.c", "build/test_bsdiff.c", newsize, "build/test_patch.bin");
    TEST_ASSERT_EQUAL(0, bspatch_result);
    /* verify the new file is the same as the original */
    const int cmp_result = cmp("main/test_bsdiff.c", "build/test_bsdiff.c");
    TEST_ASSERT_EQUAL(0, cmp_result);
}

void test_bsdiff_same_file_wrong(void)
{
    const int cmp_result = cmp("main/test_bsdiff.c", "main/CMakeLists.txt");
    TEST_ASSERT_NOT_EQUAL(0, cmp_result);
}

void test_bsdiff_different_files_oldwrong(void)
{
    /* create the patch from two well known files */
    const int newsize = bsdiff_f("main/test_bsdiff.c", "main/CMakeLists.txt", "build/test_patch.bin");
    TEST_ASSERT_GREATER_THAN(1, newsize);
    /* apply the patch on top of a bad old file*/
    const int bspatch_result = bspatch_f("main/CMakeLists.c", "build/CMakeLists.txt", newsize, "build/test_patch.bin");
    TEST_ASSERT_NOT_EQUAL(0, bspatch_result);
}

void test_bsdiff_different_files_missingfile(void)
{
    /* create the patch from two well known files */
    const int res = bsdiff_f("main/test_bsdiffMISSING.c", "main/CMakeLists.txt", "build/test_patch.bin");
    TEST_ASSERT_EQUAL(-1, res);
    const int res2 = bsdiff_f("main/test_bsdiff.c", "main/CMakeListsMISSING.txt", "build/test_patch.bin");
    TEST_ASSERT_EQUAL(-1, res2);
}

int main(int argc, char** argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_bsdiff_different_files);
    RUN_TEST(test_bsdiff_same_file);
    RUN_TEST(test_bsdiff_same_file_wrong);
    RUN_TEST(test_bsdiff_different_files_oldwrong);
    RUN_TEST(test_bsdiff_different_files_missingfile);
    int failures = UNITY_END();
    return failures;
}
