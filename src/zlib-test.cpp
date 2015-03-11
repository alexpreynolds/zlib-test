#include "zlib-test.h"

int
main(int argc, char** argv)
{
    if ((argc == 2) && (zlibTest::ZlibTest::is_regular_file(argv[1]))) {
        zlibTest::ZlibTest *ztest = new zlibTest::ZlibTest(argv[1]);
        ztest->init_z_stream_ptr();
        ztest->compress_in_fn();
        ztest->delete_z_stream_ptr();
        delete ztest;
    }
    else {
        zlibTest::ZlibTest::print_usage();
    }

    return EXIT_SUCCESS;
}

void 
zlibTest::ZlibTest::open_fp(std::string _fn, FILE** _fp_ptr, std::string _mode)
{
    try {
        if (!(*_fp_ptr = std::fopen(_fn.c_str(), _mode.c_str()))) {
            throw std::runtime_error("could not open file pointer [" + _fn + "]");
        }
    }
    catch (std::runtime_error &e) {
        fprintf(stderr, "Error: %s\n", e.what());
    }    
}

void 
zlibTest::ZlibTest::close_fp(FILE** _fp_ptr)
{
    try {
        if (std::fclose(*_fp_ptr) != 0) {
            throw std::runtime_error("could not close file pointer");
        }
    }
    catch (std::runtime_error &e) {
        fprintf(stderr, "Error: %s\n", e.what());
    }
}

void 
zlibTest::ZlibTest::compress_in_fn(void)
{
    // set up handles
    std::string in_mode = "r";
    std::string out_mode = "wb";
    this->open_fp(this->in_fn, &this->in_fp, in_mode);
    this->open_fp(this->out_fn, &this->out_fp, out_mode);

    // init deflate on stream
    int z_error = -1;
    try {
        z_error = deflateInit2(this->z_stream_ptr, 
                               ZLIB_TEST_COMPRESSION_LEVEL, 
                               ZLIB_TEST_COMPRESSION_METHOD, 
                               ZLIB_TEST_COMPRESSION_WINDOW_BITS,
                               ZLIB_TEST_COMPRESSION_MEM_LEVEL,
                               ZLIB_TEST_COMPRESSION_STRATEGY);
        // the following does not make gzip-compatible archive
        // z_error = deflateInit(this->z_stream_ptr, ZLIB_TEST_COMPRESSION_LEVEL);
        switch(z_error) {
        case Z_MEM_ERROR:
            throw std::runtime_error("Not enough memory is available");
        case Z_STREAM_ERROR:
            throw std::runtime_error("Gzip initialization parameter is invalid (e.g., invalid method)");
        case Z_VERSION_ERROR:
            throw std::runtime_error("The zlib library version is incompatible with the version assumed by the caller (ZLIB_VERSION)");
        case Z_OK:
        default:
            break;
        }
    }
    catch (std::runtime_error &e) {
        fprintf(stderr, "Error: %s\n", e.what());
    }

    // deflate in_fp
    int ret, flush;
    unsigned int have;
    unsigned char in_buf[ZLIB_TEST_CHUNK];
    unsigned char out_buf[ZLIB_TEST_CHUNK];
    
    do {
        this->z_stream_ptr->avail_in = static_cast<unsigned int>( std::fread(in_buf, 1, ZLIB_TEST_CHUNK, this->in_fp) );
        flush = feof(this->in_fp) ? Z_FINISH : Z_NO_FLUSH;
        this->z_stream_ptr->next_in = in_buf;

        // read in all of in_buf into out_buf
        try {
            do {
                this->z_stream_ptr->avail_out = ZLIB_TEST_CHUNK;
                this->z_stream_ptr->next_out = out_buf;
                try {
                    ret = deflate(this->z_stream_ptr, flush);
                    if (ret == Z_STREAM_ERROR) {
                        throw std::runtime_error("Z_STREAM_ERROR found");
                    }
                }
                catch (std::runtime_error &e) {
                    fprintf(stderr, "Error: %s\n", e.what());
                }
                have = ZLIB_TEST_CHUNK - this->z_stream_ptr->avail_out;
                // write "have" bytes from out_buf to out_fp
                if (std::fwrite(out_buf, 1, have, this->out_fp) != have || std::ferror(this->out_fp)) {
                    throw std::runtime_error("Compression error Z_ERRNO [" + std::to_string(Z_ERRNO) + "]");
                }
            } while (this->z_stream_ptr->avail_out == 0);
        }
        catch (std::runtime_error &e) {
            fprintf(stderr, "Error: %s\n", e.what());
        }
    } while (flush != Z_FINISH);

    // close handles
    this->close_fp(&this->out_fp);
    this->close_fp(&this->in_fp);
}

void 
zlibTest::ZlibTest::init_z_stream_ptr(void) 
{
    try {
        this->z_stream_ptr = new z_stream;
        }
    catch (std::bad_alloc& ba) {
        std::fprintf(stderr, "Error: Bad alloc caught: %s\n", ba.what());
    }
    this->z_stream_ptr->zalloc = Z_NULL;
    this->z_stream_ptr->zfree = Z_NULL;
    this->z_stream_ptr->opaque = Z_NULL;
}

void 
zlibTest::ZlibTest::delete_z_stream_ptr(void) 
{
    delete this->z_stream_ptr;
}
