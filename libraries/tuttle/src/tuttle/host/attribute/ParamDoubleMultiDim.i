%include <tuttle/host/global.i>

%include <tuttle/host/ofx/attribute/OfxhParam.i>
%include <tuttle/host/ofx/attribute/OfxhMultiDimParam.i>
%include <tuttle/host/ofx/attribute/OfxhParamAccessor.i>
%include <tuttle/host/ofx/attribute/OfxhParamDescriptor.i>

%{
#include <tuttle/host/ofx/attribute/OfxhParam.hpp>
#include <tuttle/host/ofx/attribute/OfxhParamAccessor.hpp>
#include <tuttle/host/ofx/attribute/OfxhParamDescriptor.hpp>

#include <tuttle/host/attribute/ParamDoubleMultiDim.hpp>
%}

%include <tuttle/host/attribute/ParamDoubleMultiDim.hpp>

