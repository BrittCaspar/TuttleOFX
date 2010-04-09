#include "DPXReaderPlugin.hpp"
#include "DPXReaderDefinitions.hpp"

#include <tuttle/common/image/gilGlobals.hpp>
#include <tuttle/common/image/gilViewTypes.hpp>
#include <tuttle/plugin/ImageGilProcessor.hpp>
#include <tuttle/plugin/PluginException.hpp>

#include <cstdlib>
#include <cassert>
#include <cmath>
#include <vector>
#include <iostream>
#include <ofxsImageEffect.h>
#include <ofxsMultiThread.h>
#include <boost/gil/gil_all.hpp>
#include <boost/gil/packed_pixel.hpp>

#include <boost/integer.hpp>  // for boost::uint_t
#include <boost/cstdint.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/filesystem/fstream.hpp>

namespace tuttle {
namespace plugin {
namespace dpx {
namespace reader {

namespace bfs = boost::filesystem;

template<class View>
DPXReaderProcess<View>::DPXReaderProcess( DPXReaderPlugin& instance )
	: ImageGilProcessor<View>( instance ),
	_plugin( instance )
{
}

template<class View>
void DPXReaderProcess<View>::setup( const OFX::RenderArguments& args )
{
	using namespace boost::gil;
	ImageGilProcessor<View>::setup( args );
	DPXReaderParams params = _plugin.getParams(args.time);
	if( ! bfs::exists( params._filepath ) )
	{
		throw( PluginException( "Unable to open : " + params._filepath ) );
	}
	_plugin.getDpxImg().read( params._filepath, true );
}

/**
 * @brief Function called by rendering thread each time a process must be done.
 * @param[in] procWindowRoW  Processing window in RoW
 */
template<class View>
void DPXReaderProcess<View>::multiThreadProcessImages( const OfxRectI& procWindowRoW )
{
	using namespace boost::gil;
	try
	{
		DPXReaderParams params = _plugin.getParams(this->_renderArgs.time);
		if( bfs::exists( params._filepath ) )
		{
			readImage( this->_dstView, params._filepath );
		}
		else
		{
			throw( PluginException( "Unable to open : " + params._filepath ) );
		}
	}
	catch( tuttle::plugin::PluginException& e )
	{
		COUT_EXCEPTION( e );
	}
}

template<class View>
View& DPXReaderProcess<View>::readImage( View& dst, std::string& filepath ) throw( tuttle::plugin::PluginException )
{
	using namespace std;
	using namespace boost;
	using namespace mpl;
	using namespace boost::gil;

	switch( _plugin.getDpxImg().componentsType() )
	{
		case tuttle::io::DpxImage::eCompTypeR8G8B8:
		{
			// Tests passed: fill, non fill, big endian, little endian
			rgb8c_view_t src = interleaved_view( _plugin.getDpxImg().width(), _plugin.getDpxImg().height(),
			                                     (const rgb8_pixel_t*)( _plugin.getDpxImg().data() ),
			                                     _plugin.getDpxImg().width() * 3 );
			copy_and_convert_pixels( flipped_up_down_view( src ), dst );
			break;
		}
		case tuttle::io::DpxImage::eCompTypeR8G8B8A8:
		{
			// Tests passed: fill, non fill, big endian, little endian
			rgba8c_view_t src = interleaved_view( _plugin.getDpxImg().width(), _plugin.getDpxImg().height(),
			                                      (const rgba8_pixel_t*)( _plugin.getDpxImg().data() ),
			                                      _plugin.getDpxImg().width() * 4 );

			copy_and_convert_pixels( flipped_up_down_view( src ), dst );
			break;
		}
		case tuttle::io::DpxImage::eCompTypeA8B8G8R8:
		{
			// Untested (need images samples), quite sure it is working
			abgr8c_view_t src = interleaved_view( _plugin.getDpxImg().width(), _plugin.getDpxImg().height(),
			                                      (const abgr8_pixel_t*)( _plugin.getDpxImg().data() ),
			                                      _plugin.getDpxImg().width() * 4 );

			copy_and_convert_pixels( flipped_up_down_view( src ), dst );
			break;
		}
		case tuttle::io::DpxImage::eCompTypeR10G10B10:
		{
			// Tests passed: fill, non fill, big endian, little endian
			// Interpret pixels according to its bit packing
			switch( _plugin.getDpxImg().packing() )
			{
				// bit stream
				case 0:
				{
					rgb16_image_t img( dst.width(), dst.height() );
					rgb16_view_t vw( view( img ) );
					bitStreamToView<rgb10_stream_ptr_t>( vw, 3, 10 );
					copy_and_convert_pixels( vw, flipped_up_down_view( dst ) );
					break;
				}
				default:
				{
					int width               = _plugin.getDpxImg().width();
					int height              = _plugin.getDpxImg().height();
					rgb10_packed_view_t src = interleaved_view( width, height,
					                                            ( rgb10_packed_pixel_t* )( _plugin.getDpxImg().data() ),
					                                            width * sizeof( uint32_t ) );
					// This is temporary but needed because of a probable bug in gil
					// Should be using copy_and_convert_pixels
					rgb16_image_t img16( width, height );
					rgb16_view_t vw16( view( img16 ) );
					for( typename rgb10_packed_view_t::y_coord_t y = 0; y < height; ++y )
					{
						typename rgb10_packed_view_t::x_iterator sit = src.row_begin( y );
						typename rgb16_view_t::x_iterator dit        = vw16.row_begin( y );
						for( typename rgb10_packed_view_t::x_coord_t x = 0; x < width; ++x )
						{
							color_convert( *sit, *dit );
							++sit;
							++dit;
						}
					}
					copy_and_convert_pixels( vw16, flipped_up_down_view( dst ) );
					break;
				}
			}
			break;
		}
		case tuttle::io::DpxImage::eCompTypeR10G10B10A10:
		{
			// Tests passed: fill with big endian or little endian
			// Tests failed: non fill
			// Interpret pixels according to its bit packing
			switch( _plugin.getDpxImg().packing() )
			{
				// bit stream
				case 0:
				{
					rgba16_image_t img( dst.width(), dst.height() );
					rgba16_view_t vw( view( img ) );
					bitStreamToView<rgba10_stream_ptr_t>( vw, 4, 10 );
					copy_and_convert_pixels( vw, flipped_up_down_view( dst ) );
					break;
				}
				default:
				{
					throw( PluginException( "Error: unsupported dpx file format (RGBA10 byte packed). " ) );
					break;
				}
			}
			break;
		}
		case tuttle::io::DpxImage::eCompTypeA10B10G10R10:
		{
			// Untested (need images samples), quite sure it is not working :(
			// Interpret pixels according to its bit packing
			switch( _plugin.getDpxImg().packing() )
			{
				// bit stream
				case 0:
				{
					rgba16_image_t img( dst.width(), dst.height() );
					rgba16_view_t vw( view( img ) );
					bitStreamToView<abgr10_stream_ptr_t>( vw, 4, 10 );
					copy_and_convert_pixels( vw, flipped_up_down_view( dst ) );
					break;
				}
				case 1:
				case 2:
				{
					throw( PluginException( "Error: unsupported dpx file format (ABGR10 byte packed). " ) );
					break;
				}
			}
			break;
		}
		case tuttle::io::DpxImage::eCompTypeR12G12B12:
		{
			// Tests failed: all (sick) !
			// Probable bug in gil...
			// Interpret pixels according to its bit packing
			switch( _plugin.getDpxImg().packing() )
			{
				// bit stream
				case 0:
				{
					rgb16_image_t img( dst.width(), dst.height() );
					rgb16_view_t vw( view( img ) );
					bitStreamToView<rgb12_stream_ptr_t>( vw, 3, 12 );
					copy_and_convert_pixels( vw, flipped_up_down_view( dst ) );
					break;
				}
				default:
				{
					int width               = _plugin.getDpxImg().width();
					int height              = _plugin.getDpxImg().height();
					rgb12_packed_view_t src = interleaved_view( width, height,
					                                            ( rgb12_packed_pixel_t* )( _plugin.getDpxImg().data() ),
					                                            _plugin.getDpxImg().width() * 6 );
					// This is temporary but needed because of a probable bug in gil
					// Should be using copy_and_convert_pixels
					rgb16_image_t img16( width, height );
					rgb16_view_t vw16( view( img16 ) );
					for( typename rgb12_packed_view_t::y_coord_t y = 0; y < height; ++y )
					{
						typename rgb12_packed_view_t::x_iterator sit = src.row_begin( y );
						typename rgb16_view_t::x_iterator dit        = vw16.row_begin( y );
						for( typename rgb12_packed_view_t::x_coord_t x = 0; x < width; ++x )
						{
							color_convert( *sit, *dit );
							++sit;
							++dit;
						}
					}
					copy_and_convert_pixels( vw16, flipped_up_down_view( dst ) );
					break;
				}
			}
			break;
		}
		case tuttle::io::DpxImage::eCompTypeR12G12B12A12:
		{
			// Tests passed: fill big endian or little endian, non fill little endian or big endian
			// Interpret pixels according to its bit packing
			switch( _plugin.getDpxImg().packing() )
			{
				// bit stream
				case 0:
				{
					rgba16_image_t img( dst.width(), dst.height() );
					rgba16_view_t vw( view( img ) );
					bitStreamToView<rgba12_stream_ptr_t>( vw, 4, 12 );
					copy_and_convert_pixels( vw, flipped_up_down_view( dst ) );
					break;
				}
				default:
				{
					rgba12_packed_view_t vw = interleaved_view( _plugin.getDpxImg().width(), _plugin.getDpxImg().height(),
					                                            ( rgba12_packed_pixel_t* )( _plugin.getDpxImg().data() ),
					                                            _plugin.getDpxImg().width() * sizeof( uint64_t ) );

					copy_and_convert_pixels( vw, flipped_up_down_view( dst ) );
					break;
				}
			}
			break;
		}
		case tuttle::io::DpxImage::eCompTypeA12B12G12R12:
		{
			// Untested
			// Interpret pixels according to its bit packing
			switch( _plugin.getDpxImg().packing() )
			{
				// bit stream
				case 0:
				{
					rgba16_image_t img( dst.width(), dst.height() );
					rgba16_view_t vw( view( img ) );
					bitStreamToView<abgr12_stream_ptr_t>( vw, 4, 12 );
					copy_and_convert_pixels( vw, flipped_up_down_view( dst ) );
					break;
				}
				default:
				{
					abgr12_packed_view_t vw = interleaved_view( _plugin.getDpxImg().width(), _plugin.getDpxImg().height(),
					                                            ( abgr12_packed_pixel_t* )( _plugin.getDpxImg().data() ),
					                                            _plugin.getDpxImg().width() * sizeof( uint64_t ) );

					copy_and_convert_pixels( vw, flipped_up_down_view( dst ) );
					break;
				}
			}
			break;
		}
		case tuttle::io::DpxImage::eCompTypeR16G16B16:
		{
			// Tests passed: fill, non fill, big endian, little endian
			rgb16c_view_t src = interleaved_view( _plugin.getDpxImg().width(), _plugin.getDpxImg().height(),
			                                      (const rgb16_pixel_t*)( _plugin.getDpxImg().data() ),
			                                      _plugin.getDpxImg().width() * 6 );
			copy_and_convert_pixels( flipped_up_down_view( src ), dst );
			break;
		}
		case tuttle::io::DpxImage::eCompTypeR16G16B16A16:
		{
			// Tests passed: fill, non fill, big endian, little endian
			rgba16c_view_t src = interleaved_view( _plugin.getDpxImg().width(), _plugin.getDpxImg().height(),
			                                       (const rgba16_pixel_t*)( _plugin.getDpxImg().data() ),
			                                       _plugin.getDpxImg().width() * sizeof( uint64_t ) );

			copy_and_convert_pixels( flipped_up_down_view( src ), dst );
			break;
		}
		case tuttle::io::DpxImage::eCompTypeA16B16G16R16:
		{
			// Untested (need images samples), quite sure it is working
			abgr16c_view_t src = interleaved_view( _plugin.getDpxImg().width(), _plugin.getDpxImg().height(),
			                                       (const abgr16_pixel_t*)( _plugin.getDpxImg().data() ),
			                                       _plugin.getDpxImg().width() * sizeof( uint64_t ) );

			copy_and_convert_pixels( flipped_up_down_view( src ), dst );
			break;
		}
		default:
			break;
	}

	return dst;
}

template<class View>
template<class T, class DST_V>
void DPXReaderProcess<View>::bitStreamToView( DST_V& dst, const int nc, const int channelSize )
{
	boost::uint8_t* pData = _plugin.getDpxImg().data();

	typedef unsigned char byte_t;
	int width             = _plugin.getDpxImg().width();
	int height            = _plugin.getDpxImg().height();
	int scanline_in_bits  = width * nc * channelSize;
	int scanline_in_bytes = scanline_in_bits / 8;
	scanline_in_bytes += ( scanline_in_bits % 8 != 0 ) ? 1 : 0;
	T p( pData, 0 );

	for( typename DST_V::y_coord_t y = 0; y < height; ++y )
	{
		typename DST_V::x_iterator it = dst.row_begin( y );

		for( typename DST_V::x_coord_t x = 0; x < width; ++x )
		{
			color_convert( *p, *it );
			++p;
			++it;
		}
	}
}

}
}
}
}
