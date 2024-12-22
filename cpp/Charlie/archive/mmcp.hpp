#pragma once


namespace Charlie {

template<typename DestPtr, typename SourPtr>
inline void mmcp(DestPtr dest, SourPtr src, std::size_t Nbyte)
{ 
  std::memcpy((void*)dest, (void*)src, Nbyte);
}




// template<typename Xptr, typename Yptr, std::size_t bufferNbyte>
// inline void swapBuffer(Xptr x, Yptr y)
// {
//   
// }


}