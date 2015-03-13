#include "zlib-test.h"

int
main(int argc, char** argv)
{
    if ((argc == 2) && (zlibTest::ZlibTest::is_regular_file(argv[1]))) {
        // compress (deflate)
        std::string compress_if = argv[1];
        std::string compress_of = compress_if + ZLIB_TEST_DEFAULT_EXT;
        zlibTest::ZlibTest *compress_test = new zlibTest::ZlibTest(compress_if, compress_of);
        fprintf(stderr, "Debug: Compressing [%s] to [%s] ...\n", compress_test->in_fn.c_str(), compress_test->out_fn.c_str());
        compress_test->init_z_stream_ptr();
        compress_test->compress_in_fn();
        compress_test->delete_z_stream_ptr();
        delete compress_test;
        
        // extract (inflate)
        std::string extract_if = compress_of;
        std::string extract_of = compress_of + ZLIB_TEST_EXTRACT_EXT;
        zlibTest::ZlibTest *extract_test = new zlibTest::ZlibTest(extract_if, extract_of);
        extract_test->init_z_stream_ptr();
        extract_test->extract_in_fn();
        extract_test->delete_z_stream_ptr();
        delete extract_test;
    }
    else {
        zlibTest::ZlibTest::print_usage();
    }

    return EXIT_SUCCESS;
}

void
zlibTest::ZlibTest::read_z_chunk(void)
{
    this->z_stream_ptr->avail_in = static_cast<unsigned int>( std::fread(this->in_buf, 1, ZLIB_TEST_CHUNK/2, this->in_fp) );
    this->z_stream_ptr->next_in = this->in_buf;
    try {
        int ret = -1;
        do {
            this->z_stream_ptr->avail_out = ZLIB_TEST_CHUNK;
            this->z_stream_ptr->next_out = this->out_buf;
            ret = inflate(this->z_stream_ptr, Z_NO_FLUSH);
        } while (this->z_stream_ptr->avail_out == 0);
        switch (ret) {
        case Z_NEED_DICT:
            ret = Z_DATA_ERROR;
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            inflateEnd(this->z_stream_ptr);
            throw std::runtime_error("inflate to stream failed [" + std::to_string(ret) + "]");
        }
        this->out_have = ZLIB_TEST_CHUNK - this->z_stream_ptr->avail_out;
        fprintf(stderr, "Debug: Able to write %d bytes to output stream\n", static_cast<int>( this->out_have ));
    }
    catch (std::runtime_error &e) {
        fprintf(stderr, "Error: %s\n", e.what());
    }
}

void
zlibTest::ZlibTest::write_out_buf_line(void)
{
    unsigned int curr_pos = this->out_buf_line_start;
    unsigned char *curr_line_ptr = &this->line_buf[0];

    if (this->rem_len != 0) {
        std::memcpy(this->line_buf, this->rem_buf, this->rem_len);
        this->line_len = this->rem_len;
        this->line_buf[this->line_len] = '\0';
        curr_line_ptr = &this->line_buf[this->line_len];
    }
    
    do {
        this->out_buf_line_end = curr_pos;
        this->line_len = this->out_buf_line_end - this->out_buf_line_start + this->rem_len;
        if (this->out_buf[curr_pos] == '\n') {
            this->line_len++;
            std::memcpy(curr_line_ptr, &this->out_buf[this->out_buf_line_start], this->line_len);
            try {
                this->line_buf[this->line_len] = '\0';
                if (std::fwrite(this->line_buf, 1, this->line_len, this->out_fp) != this->line_len) {
                    inflateEnd(this->z_stream_ptr);
                    throw std::runtime_error("inflate line write failed");
                }
                this->out_buf_line_start = curr_pos + 1;
                this->line_buf[0] = '\0';
                this->line_len = 0;
                this->rem_len = 0;
                curr_line_ptr = &this->line_buf[0];
            }
            catch (std::runtime_error &e) {
                fprintf(stderr, "Error: %s\n", e.what());
            }
        }
    } while (++curr_pos < this->out_have_to_go);

    if (this->line_len != 0) {
        std::memcpy(this->rem_buf, &this->out_buf[this->out_buf_line_start], this->line_len + 1);
        this->rem_buf[this->line_len + 1] = '\0';
        this->rem_len = this->line_len + 1;
    }

    this->out_have_to_go = 0;
}

void
zlibTest::ZlibTest::extract_in_fn(void)
{
    // set up handles
    std::string in_mode = "r";
    std::string out_mode = "wb";
    this->open_fp(this->in_fn, &this->in_fp, in_mode);
    this->open_fp(this->out_fn, &this->out_fp, out_mode);

    // initialize extraction machinery
    try {
        int ret = inflateInit2(this->z_stream_ptr, ZLIB_TEST_COMPRESSION_WINDOW_BITS + 16);
        //int ret = inflateInit(this->z_stream_ptr);
        if (ret != Z_OK)
            throw std::runtime_error("could not initialize initialization machinery");
        this->rem_len = 0;
        this->line_len = 0;
        // keep reading until this->out_have == 0
        do {
            this->read_z_chunk();
            // write out data one line at a time
            this->out_have_to_go = this->out_have;
            this->out_buf_line_start = 0;
            this->out_buf_line_end = 0;
            do {
                this->write_out_buf_line();
            } while (this->out_have_to_go > 0);
        } while (this->out_have != 0);
    }
    catch (std::runtime_error &e) {
        fprintf(stderr, "Error: %s\n", e.what());
    }

    // break down extraction machinery
    inflateEnd(this->z_stream_ptr);

    // close handles
    this->close_fp(&this->out_fp);
    this->close_fp(&this->in_fp);    
}

void 
zlibTest::ZlibTest::open_fp(std::string _fn, FILE** _fp_ptr, std::string _mode)
{
    try {
        if (!(*_fp_ptr = std::fopen(_fn.c_str(), _mode.c_str()))) {
            throw std::runtime_error("could not open file pointer to file [" + _fn + "]");
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
        // the following makes a gzip-compatible archive, but requires adding 16 in inflateInit2() call (see above)
        /*
        z_error = deflateInit2(this->z_stream_ptr, 
                               ZLIB_TEST_COMPRESSION_LEVEL, 
                               ZLIB_TEST_COMPRESSION_METHOD, 
                               ZLIB_TEST_COMPRESSION_WINDOW_BITS,
                               ZLIB_TEST_COMPRESSION_MEM_LEVEL,
                               ZLIB_TEST_COMPRESSION_STRATEGY);
        */
        // the following does not make gzip-compatible archive -- what header does it write?
        z_error = deflateInit(this->z_stream_ptr, ZLIB_TEST_COMPRESSION_LEVEL);
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
    
    do {
        this->z_stream_ptr->avail_in = static_cast<unsigned int>( std::fread(this->in_buf, 1, ZLIB_TEST_CHUNK, this->in_fp) );
        flush = feof(this->in_fp) ? Z_FINISH : Z_NO_FLUSH;
        this->z_stream_ptr->next_in = this->in_buf;

        // read in all of in_buf into out_buf
        try {
            do {
                this->z_stream_ptr->avail_out = ZLIB_TEST_CHUNK;
                this->z_stream_ptr->next_out = this->out_buf;
                try {
                    ret = deflate(this->z_stream_ptr, flush);
                    if (ret == Z_STREAM_ERROR) {
                        throw std::runtime_error("Z_STREAM_ERROR found");
                    }
                }
                catch (std::runtime_error &e) {
                    fprintf(stderr, "Error: %s\n", e.what());
                }
                this->out_have = ZLIB_TEST_CHUNK - this->z_stream_ptr->avail_out;
                // write "this->have" bytes from out_buf to out_fp
                if (std::fwrite(this->out_buf, 1, this->out_have, this->out_fp) != this->out_have || std::ferror(this->out_fp)) {
                    throw std::runtime_error("Compression error Z_ERRNO [" + std::to_string(Z_ERRNO) + "]");
                }
            } while (this->z_stream_ptr->avail_out == 0);
        }
        catch (std::runtime_error &e) {
            fprintf(stderr, "Error: %s\n", e.what());
        }
    } while (flush != Z_FINISH);

    // break down deflation machinery
    deflateEnd(this->z_stream_ptr);

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
    this->z_stream_ptr->avail_in = 0;
    this->z_stream_ptr->next_in = Z_NULL;
    this->z_stream_ptr->avail_out = 0;
    this->z_stream_ptr->next_out = Z_NULL;
}

void 
zlibTest::ZlibTest::delete_z_stream_ptr(void) 
{
    delete this->z_stream_ptr;
}
