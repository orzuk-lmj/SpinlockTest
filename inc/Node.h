#pragma once

template<typename _Ty>
struct NodeT
{
    //long long p1, p2, p3, p4, p5, p6, p7; // cache line padding
    std::unique_ptr<_Ty> data;
    NodeT* next;
    //long long p8, p9, p10, p11, p12, p13, p14; // cache line padding

    NodeT(const _Ty& _val, NodeT* _next) : data(new _Ty(_val)), next(_next) {}
};
