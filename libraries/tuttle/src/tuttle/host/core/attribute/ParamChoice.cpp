#include "ParamChoice.hpp"

namespace tuttle {
namespace host {
namespace core {

ParamChoice::ParamChoice( ImageEffectNode&                                   effect,
                          const std::string&                                 name,
                          const ofx::attribute::OfxhParamDescriptor& descriptor )
	: ofx::attribute::OfxhParamChoice( descriptor, name, effect ),
	_effect( effect )

{
	_value = getDefault();
}

int ParamChoice::getDefault() const
{
	return getProperties().getIntProperty( kOfxParamPropDefault );
}

void ParamChoice::get( int& v ) const OFX_EXCEPTION_SPEC
{
	v = _value;
}

void ParamChoice::getAtTime( const OfxTime time, int& v ) const OFX_EXCEPTION_SPEC
{
	v = _value; ///< @todo: in time !
}

void ParamChoice::set( const int &v, const ofx::attribute::EChange change ) OFX_EXCEPTION_SPEC
{
	_value = v;
	paramChanged( change );
}

void ParamChoice::setAtTime( const OfxTime time, const int &v, const ofx::attribute::EChange change ) OFX_EXCEPTION_SPEC
{
	_value = v; ///< @todo: in time !
	paramChanged( change );
}

void ParamChoice::copy( const ParamChoice& p ) OFX_EXCEPTION_SPEC
{
	_value = p._value;
//	paramChanged( ofx::attribute::eChangeUserEdited );
}

void ParamChoice::copy( const OfxhParam& p ) OFX_EXCEPTION_SPEC
{
	const ParamChoice& param = dynamic_cast<const ParamChoice&>(p);
	copy( param );
}


}
}
}

