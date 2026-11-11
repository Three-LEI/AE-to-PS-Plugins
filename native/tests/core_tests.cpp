#include "tint_core.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

void expect(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
template<class T> tint::SampleType type();
template<> tint::SampleType type<unsigned char>() { return tint::SampleType::u8; }
template<> tint::SampleType type<unsigned short>() { return tint::SampleType::photoshop_u16; }
template<> tint::SampleType type<float>() { return tint::SampleType::f32; }
template<class T> void formats(double maximum) {
    for (unsigned channels : {3u,4u}) {
        constexpr std::size_t width=19, height=7;
        const auto stride = width*channels*sizeof(T)+13;
        std::vector<unsigned char> source(stride*height+3,0xCD), destination(stride*height+5,0xEF);
        for (std::size_t y=0;y<height;++y) for(std::size_t x=0;x<width;++x) for(unsigned c=0;c<channels;++c) {
            const T value=static_cast<T>(maximum*static_cast<double>((x*19+y*13+c*29)%101)/100.0);
            std::memcpy(source.data()+1+y*stride+(x*channels+c)*sizeof(T),&value,sizeof(T));
        }
        tint::ConstImage input{source.data()+1,stride*height,width,height,stride,channels,type<T>()};
        tint::Image output{destination.data()+2,stride*height,width,height,stride,channels,type<T>()};
        tint::Parameters p;
        p.black[0]=0.1; p.black[1]=0.3; p.black[2]=0.2;
        p.white[0]=0.9; p.white[1]=0.7; p.white[2]=1.0;
        const auto original=source;
        expect(tint::process(input,output,p)==tint::Status::ok,"render failed");
        expect(source==original,"out-of-place source changed");
        // Verify alpha bits and all padding/guard bytes, including unaligned rows.
        expect(destination[0]==0xEF && destination[1]==0xEF,"leading guard overwritten");
        for(std::size_t y=0;y<height;++y) {
            for (std::size_t b=width*channels*sizeof(T);b<stride;++b) expect(destination[2+y*stride+b]==0xEF,"padding overwritten");
            if(channels==4) for(std::size_t x=0;x<width;++x) {
                const auto offset=y*stride+(x*channels+3)*sizeof(T);
                expect(std::memcmp(source.data()+1+offset,destination.data()+2+offset,sizeof(T))==0,"alpha changed");
            }
        }
        for(std::size_t b=2+stride*height;b<destination.size();++b) expect(destination[b]==0xEF,"trailing guard overwritten");
        auto inplace=source;
        auto in_place_input=input; in_place_input.data=inplace.data()+1;
        auto in_place_output=output; in_place_output.data=inplace.data()+1;
        expect(tint::process(in_place_input,in_place_output,p)==tint::Status::ok,"in-place failed");
        for(std::size_t y=0;y<height;++y) expect(std::memcmp(inplace.data()+1+y*stride,destination.data()+2+y*stride,width*channels*sizeof(T))==0,"in-place mismatch");
        p.amount=0;
        expect(tint::process(input,output,p)==tint::Status::ok,"zero amount failed");
        for(std::size_t y=0;y<height;++y) expect(std::memcmp(source.data()+1+y*stride,destination.data()+2+y*stride,width*channels*sizeof(T))==0,"zero amount not exact identity");
        auto partial=output; partial.data=source.data()+2;
        expect(tint::process(input,partial,p)==tint::Status::overlapping_buffers,"partial alias accepted");
        auto short_buffer=input; short_buffer.capacity=1;
        expect(tint::process(short_buffer,output,p)==tint::Status::invalid_layout,"short buffer accepted");
    }
}
int main() {
    try {
        formats<unsigned char>(255); formats<unsigned short>(32768); formats<float>(1);
        unsigned char source[]={0,0,0,255,255,255}, output[6]{};
        tint::ConstImage in{source,6,2,1,6,3,tint::SampleType::u8};
        tint::Image out{output,6,2,1,6,3,tint::SampleType::u8};
        tint::Parameters p; p.black[0]=16./255; p.black[1]=32./255; p.black[2]=64./255;
        p.white[0]=1; p.white[1]=192./255; p.white[2]=96./255;
        expect(tint::process(in,out,p)==tint::Status::ok,"endpoints failed");
        const unsigned char expected[]={16,32,64,255,192,96};
        expect(std::memcmp(output,expected,6)==0,"endpoints incorrect");
        p.amount=std::numeric_limits<double>::quiet_NaN();
        expect(tint::process(in,out,p)==tint::Status::invalid_parameters,"NaN amount accepted");
        p.amount=1;
        auto overflow=in; overflow.width=std::numeric_limits<std::size_t>::max();
        auto overflow_out=out; overflow_out.width=overflow.width;
        expect(tint::process(overflow,overflow_out,p)==tint::Status::invalid_layout,"size overflow accepted");
        float hdr[]={2,2,2,0.4f,-0.25f,-0.25f,-0.25f,0.8f}, hdr_out[8]{};
        tint::ConstImage hdr_in{hdr,sizeof(hdr),2,1,sizeof(hdr),4,tint::SampleType::f32};
        tint::Image hdr_dest{hdr_out,sizeof(hdr_out),2,1,sizeof(hdr_out),4,tint::SampleType::f32};
        expect(tint::process(hdr_in,hdr_dest,{})==tint::Status::ok,"HDR failed");
        expect(hdr_out[0]>1.99f && hdr_out[4]<-0.24f,"HDR clipped");
        std::cout<<"PASS: RGB/RGBA x 8/PS16/float32; endpoints; exact 0%; alpha; in-place; unaligned padded rows; guards; invalid layout; overflow; HDR.\n";
        const std::size_t width=3840,height=2160,row=width*4;
        std::vector<unsigned char> pixels(row*height), result(row*height);
        for(std::size_t i=0;i<pixels.size();++i) pixels[i]=static_cast<unsigned char>(i*37+19);
        tint::ConstImage bench_in{pixels.data(),pixels.size(),width,height,row,4,tint::SampleType::u8};
        tint::Image bench_out{result.data(),result.size(),width,height,row,4,tint::SampleType::u8};
        std::vector<double> times;
        for(int trial=0;trial<8;++trial) {
            auto start=std::chrono::steady_clock::now();
            expect(tint::process(bench_in,bench_out,p)==tint::Status::ok,"benchmark failed");
            const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
            if(trial) times.push_back(ms);
        }
        std::sort(times.begin(),times.end());
        std::uint64_t checksum=0; for(auto b:result) checksum+=b;
        std::cout<<"BENCH: 3840x2160 RGBA8, one CPU thread, 7 timed runs, median_ms="<<times[3]<<", checksum="<<checksum<<"\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<"FAIL: "<<e.what()<<"\n"; return 1; }
}
