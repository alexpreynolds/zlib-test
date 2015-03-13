#ifndef ZLIB_TEST_H_
#define ZLIB_TEST_H_

#define ZLIB_TEST_NAME "zlib-test"
#define ZLIB_TEST_VERSION "0.1"
#define ZLIB_TEST_AUTHORS "Alex Reynolds"

#define ZLIB_TEST_CHUNK 1024
#define ZLIB_TEST_COMPRESSION_LEVEL 1
#define ZLIB_TEST_COMPRESSION_METHOD Z_DEFLATED
#define ZLIB_TEST_COMPRESSION_WINDOW_BITS 15+16
#define ZLIB_TEST_COMPRESSION_MEM_LEVEL 8
#define ZLIB_TEST_COMPRESSION_STRATEGY Z_DEFAULT_STRATEGY
#define ZLIB_TEST_DEFAULT_EXT ".zlib-test"
#define ZLIB_TEST_EXTRACT_EXT ".out"

#include <exception>
#include <stdexcept>
#include <string>
#include <cstdlib>
#include <cstdio>
#include "sys/stat.h"
#include "zlib/zlib.h"

namespace zlibTest
{
    class ZlibTest
    {
    public:
        ZlibTest(std::string, std::string);
        ~ZlibTest();

        std::string in_fn;
        std::string out_fn;
        FILE* in_fp;
        FILE* out_fp;
        z_stream* z_stream_ptr;
        unsigned char in_buf[ZLIB_TEST_CHUNK];
        unsigned int out_have;
        unsigned int out_have_to_go;
        unsigned char out_buf[ZLIB_TEST_CHUNK];
        unsigned int out_buf_line_start;
        unsigned int out_buf_line_end;
        unsigned char line_buf[ZLIB_TEST_CHUNK];
        unsigned int line_len;
        unsigned char rem_buf[ZLIB_TEST_CHUNK];
        unsigned int rem_len;

        void read_z_chunk(void);
        void write_out_buf_line(void);
        void extract_in_fn(void);
        void open_fp(std::string _fn, FILE** _fp_ptr, std::string _mode);
        void close_fp(FILE** _fp_ptr);
        void compress_in_fn(void);
        void init_z_stream_ptr(void);
        void delete_z_stream_ptr(void);

        static void print_usage(void) { 
            std::fprintf(stderr, "Usage: %s some_file\n", ZLIB_TEST_NAME); 
        }

        static bool is_regular_file(std::string _fn) {  
            struct stat st;
            try {
                if (stat(_fn.c_str(), &st) == -1) {
                    throw std::runtime_error("stat failed on regular file test");
                }
            }
            catch (std::runtime_error &e) {
                fprintf(stderr, "Error: %s\n", e.what()); 
            }
            return S_ISREG(st.st_mode);
        }
    };

    ZlibTest::ZlibTest(std::string _if, std::string _of) {
        fprintf(stderr, "Debug: Setting up ztest object...\n");
        in_fn = _if;
        out_fn = _of;
        in_fp = NULL;
        out_fp = NULL;
        fprintf(stderr, "Debug: Set up ztest object\n");
    }

    ZlibTest::~ZlibTest() {
        fprintf(stderr, "Debug: Breaking down ztest object...\n");
        fprintf(stderr, "Debug: Broken down ztest object\n");
    }
}

#endif // ZLIB_TEST_H_
