
/*
 *      ** libPKT - A tiny packet serialization library (C++/11) **
 *
 *                  Copyright (c) 2026 Nathan Volpato
*/


#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>


#define PKT_HEADER_MAGIC_BYTES                  0x21544b50
#define PKT_HEADER_MAGIC_SIZE                   0x04
#define PKT_HEADER_SECTION_DIVIDER              0x3a
#define PKT_HEADER_OFFSET_ENTRY_BYTES           (sizeof(U8) + sizeof(U16))        // 3 bytes (24 bits)

#define BYTE                                uint8_t             // 0
#define STR                                 std::string         // 1
#define POOLSTR                             std::vector<STR>    // 2

#define I8                                  int8_t              // 3
#define I16                                 int16_t             // 4
#define I32                                 int32_t             // 5
#define I64                                 int64_t             // 6

#define U8                                  uint8_t             // 7
#define U16                                 uint16_t            // 8
#define U32                                 uint32_t            // 9
#define U64                                 uint64_t            // 10

#define F32                                 float               // 11
#define F64                                 double              // 12

#define TYPE_BYTE                           0
#define TYPE_STR                            1
#define TYPE_POOLSTR                        2

#define TYPE_I8                             3
#define TYPE_I16                            4
#define TYPE_I32                            5
#define TYPE_I64                            6

#define TYPE_U8                             7
#define TYPE_U16                            8
#define TYPE_U32                            9
#define TYPE_U64                            10

#define TYPE_F32                            11
#define TYPE_F64                            12


struct PKTByteVector {

    typedef U64 size_type;

private:
    std::vector<BYTE> _data;

public:
    inline PKTByteVector() { }
    
    inline PKTByteVector(const size_type &p_size) {
        _data.reserve(p_size);
    }

public:
    inline size_type plus_alloc(size_type p_count) {        // resizes vector to +p_count
        size_type c = _data.size();
        size_type nc = c + p_count;
        _data.resize(nc);
        return c;
    }

public:
    inline size_type size() const { return _data.size(); }
    inline std::vector<BYTE> &data() { return _data; }
    inline BYTE &operator[](const size_type &p_idx) { return (BYTE &)_data[p_idx]; }
    inline const BYTE &at(const size_type &p_idx) { return (const BYTE &)_data.at(p_idx); }

public:
    inline void append(const BYTE &p_Val) { _data.push_back(p_Val); }
    inline void append_vector(const PKTByteVector &p_Vec) {
        for (const BYTE &_El : p_Vec._data) {
            _data.push_back(_El);
        }
    }

public:
    inline bool contains(const BYTE &p_Val) const {
        for (const BYTE &_El : _data) {
            if (_El == p_Val)
                return true;
        }
        return false;
    }
    inline size_type find(const BYTE &p_Val) const {
        size_type _Idx = 0;
        for (const BYTE &_El : _data) {
            if (_El == p_Val) return _Idx;
            _Idx++;
        }
        return -1;
    }
    inline size_type count(const BYTE &p_Val) const {
        size_type _Count = 0;
        for (const BYTE &_El : _data)
            if (_El == p_Val) _Count++;
        return _Count;
    }

#define VALIDATE_SUB_RANGE(x, y, max)       ((y > x) && (x >= 0 && x <= max) && (y > 0 && y <= max))
public:
    inline PKTByteVector subv(const size_type &p_Begin, const size_type &p_End) const {
        PKTByteVector _Ret;
        size_type _Max = _data.size();
        if (!VALIDATE_SUB_RANGE(p_Begin, p_End, _Max)) {
            printf("ERROR: subv() bad range\n");
            return _Ret;
        }
        for (size_type _Idx=0; _Idx<p_End-p_Begin; _Idx++) {
            _Ret.append(_data.at(p_Begin + _Idx));
        }
        return _Ret;
    }

    inline void print_hex_table() const {
        for (const BYTE &_El : _data) {
            printf("0x%X ", _El);
        }
        printf("\n");
    }
};


#define PKT_STRUCT(m_class)                                                                                                             \
protected:                                                                                                                              \
    static U8 _bound_slot;                                                                                                              \
    static std::vector<PropertyBinds<m_class> *> _prop_binds;                                                                           \
    static std::map<U8, ByteProcessor<m_class>> _byte_procs;                                                                            \
    typedef std::unordered_map<U8, U16> Header_OffsetMap;                                                                               \
public:                                                                                                                                 \
    static std::string get_class_name() {                                                                                               \
        return std::string(#m_class);                                                                                                   \
    }                                                                                                                                   \
    static void bind_structure() {                                                                                                      \
        static bool initialized;                                                                                                        \
        if (!initialized) {                                                                                                             \
            _bound_slot = 0;                                                                                                            \
            _bind_structure();                                                                                                          \
            initialized = true;                                                                                                         \
        }                                                                                                                               \
    }                                                                                                                                   \
    static void free_static() {                                                                                                         \
        for (int i = 0; i < _prop_binds.size(); i++) {                                                                                  \
            delete _prop_binds[i];                                                                                                      \
        }                                                                                                                               \
        _prop_binds.clear();                                                                                                            \
    }                                                                                                                                   \
private:                                                                                                                                \
    static PKTByteVector _generate_header(m_class *p_obj) {                                                                             \
        PKTByteVector _to;                                                                                                              \
        PKTFormatConverter::pack_u32(PKT_HEADER_MAGIC_BYTES, &_to);                                                                     \
        U16 byte_offset = 0;                                                                                                            \
        for (const auto &_pair : _byte_procs) {                                                                                         \
            const U8 slot = _pair.first;                                                                                                \
            const ByteProcessor<m_class> &proc = _pair.second;                                                                          \
            U16 alloc_size = (U16)proc.get_alloc_size(p_obj);                                                                           \
            if (proc.is_vec()) {                                                                                                        \
                U16 elem_count = proc.get_vec_element_count(p_obj);                                                                     \
                U16 vec_alloc_end = byte_offset + (elem_count * alloc_size) - 1;                                                        \
                PKTFormatConverter::pack_byte(slot, &_to);                                                                              \
                PKTFormatConverter::pack_u16(vec_alloc_end, &_to);                                                                      \
                byte_offset += elem_count * alloc_size;                                                                                 \
            }                                                                                                                           \
            else {                                                                                                                      \
                if (proc.get_property_type() == TYPE_STR || proc.get_property_type() == TYPE_POOLSTR) {                                 \
                    U16 alloc_end = byte_offset + alloc_size - 1;                                                                       \
                    PKTFormatConverter::pack_byte(slot, &_to);                                                                          \
                    PKTFormatConverter::pack_u16(alloc_end, &_to);                                                                      \
                }                                                                                                                       \
                byte_offset += alloc_size;                                                                                              \
            }                                                                                                                           \
        }                                                                                                                               \
        PKTFormatConverter::pack_byte(PKT_HEADER_SECTION_DIVIDER, &_to);                                                                \
        return _to;                                                                                                                     \
    }                                                                                                                                   \
    static Header_OffsetMap _deserialize_header(PKTByteVector *p_data) {                                                                \
        Header_OffsetMap ret;                                                                                                           \
        if (!p_data->contains(PKT_HEADER_SECTION_DIVIDER)) {                                                                            \
            return ret;                                                                                                                 \
        }                                                                                                                               \
        U64 sep_idx = p_data->find(PKT_HEADER_SECTION_DIVIDER);                                                                         \
        PKTByteVector header_subdata = p_data->subv(PKT_HEADER_MAGIC_SIZE, sep_idx);                                                    \
        U64 serialized_data_size = header_subdata.size();                                                                               \
        U8 entry_count = U8(serialized_data_size / PKT_HEADER_OFFSET_ENTRY_BYTES);                                                      \
        if (serialized_data_size % PKT_HEADER_OFFSET_ENTRY_BYTES != 0) {                                                                \
            return ret;     /* remember to write stderr func */                                                                         \
        }                                                                                                                               \
        U8 slot, byte_offset = 0;                                                                                                       \
        for (U8 i = 0; i < entry_count; i++) {                                                                                          \
            byte_offset = i * PKT_HEADER_OFFSET_ENTRY_BYTES;                                                                            \
            slot = header_subdata.at(byte_offset + 0);                                                                                  \
            ret[slot] = PKTFormatConverter::unpack_u16(&header_subdata, byte_offset + 1);                                               \
        }                                                                                                                               \
        return ret;                                                                                                                     \
    }                                                                                                                                   \
public:                                                                                                                                 \
    static PKTByteVector serialize(m_class *p_obj) {                                                                                    \
        PKTByteVector _header, _to;                                                                                                     \
        _header = _generate_header(p_obj);                                                                                              \
        _to.append_vector(_header);                                                                                                     \
        for (const auto &_pair : _byte_procs) {                                                                                         \
            const ByteProcessor<m_class> &proc = _pair.second;                                                                          \
            proc.serialize_property(p_obj, &_to);                                                                                       \
        }                                                                                                                               \
        return _to;                                                                                                                     \
    }                                                                                                                                   \
    static void deserialize(PKTByteVector *p_data, m_class *p_obj) {                                                                    \
        I32 _magic = PKTFormatConverter::unpack_i32(p_data);                                                                            \
        if (_magic != PKT_HEADER_MAGIC_BYTES) {                                                                                         \
            return; /* stderr */                                                                                                        \
        }                                                                                                                               \
        Header_OffsetMap offsets = _deserialize_header(p_data);                                                                         \
        U64 _Max = p_data->size();                                                                                                      \
        U64 _Sep = p_data->find(PKT_HEADER_SECTION_DIVIDER);                                                                            \
        PKTByteVector struct_data = p_data->subv(_Sep + 1, _Max);                                                                       \
        U16 byte_offset = 0;                                                                                                            \
        for (auto &_pair : _byte_procs) {                                                                                               \
            U16 var_len = 0;                                                                                                            \
            if (offsets.count(_pair.first)) {                                                                                           \
                var_len = offsets[_pair.first] - byte_offset + 1;                                                                       \
            }                                                                                                                           \
            ByteProcessor<m_class> &proc = _pair.second;                                                                                \
            proc.deserialize_property(&struct_data, p_obj, &byte_offset, var_len);                                                      \
        }                                                                                                                               \
    }                                                                                                                                   \
private: // reset back to private field
// PKT_STRUCT() end

// Static proc table allocation
#define PKT_ALLOC_STATIC(m_class)                                                                                                       \
    class m_class;                                                                                                                      \
    U8 m_class::_bound_slot = 0;                                                                                                        \
    std::vector<PropertyBinds<m_class> *> m_class::_prop_binds = std::vector<PropertyBinds<m_class> *>();                               \
    std::map<U8, ByteProcessor<m_class>> m_class::_byte_procs = std::map<U8, ByteProcessor<m_class>>();                                 \

#define PKT_FREE_STATIC(m_class)        m_class::free_static()


#define _PKT_CREATE_PROPERTY_METHODS(m_Type, m_Prop)                                                                                    \
    inline void set_##m_Prop(const m_Type &p_V) { _prop_##m_Prop = p_V; }                                                               \
    inline m_Type get_##m_Prop() const { return _prop_##m_Prop; }                                                                       \

#define PKT_CREATE_PROPERTY(m_Type, m_Prop)                                                                                             \
    m_Type  _prop_##m_Prop;                                                                                                             \
    public: _PKT_CREATE_PROPERTY_METHODS(m_Type, m_Prop);                                                                               \

#define PKT_CREATE_PROPERTY_D(m_Type, m_Prop, m_Def)                                                                                    \
    m_Type  _prop_##m_Prop = (m_Type)m_Def;                                                                                             \
    public: _PKT_CREATE_PROPERTY_METHODS(m_Type, m_Prop);                                                                               \

#define PKT_CREATE_PROPERTY_POOLSTR(m_Prop)                                                                                             \
    POOLSTR _prop_##m_Prop = POOLSTR();                                                                                                 \
public:                                                                                                                                 \
    inline void set_##m_Prop(const POOLSTR &p_V) { _prop_##m_Prop = p_V; }                                                              \
    inline POOLSTR *get_##m_Prop() { return &_prop_##m_Prop; }                                                                          \

#define PKT_CREATE_PROPERTY_VEC(m_Type, m_Prop)                                                                                         \
    std::vector<m_Type> _prop_##m_Prop = std::vector<m_Type>();                                                                         \
public:                                                                                                                                 \
    inline void set_##m_Prop(const std::vector<m_Type> &p_V) { _prop_##m_Prop = p_V; }                                                  \
    inline std::vector<m_Type> *get_##m_Prop() { return &_prop_##m_Prop; }                                                              \

    
// internal bind macros
#define _PKT_BIND_PROPERTY(m_class, m_prop, m_ti, m_ts) {                                                                               \
    PropertyBinds<m_class> *binds = new PropertyBinds<m_class>;                                                                         \
    binds->property_type = m_ti;                                                                                                        \
    binds->method_set_##m_ts = &m_class::set_##m_prop;                                                                                  \
    binds->method_get_##m_ts = &m_class::get_##m_prop;                                                                                  \
    _prop_binds.push_back(binds);                                                                                                       \
    ByteProcessor bp = ByteProcessor<m_class>(binds);                                                                                   \
    _byte_procs[_bound_slot] = bp;                                                                                                      \
}

#define _PKT_BIND_PROPERTY_VEC(m_class, m_prop, m_ti, m_ts) {                                                                           \
    PropertyBinds<m_class> *binds = new PropertyBinds<m_class>;                                                                         \
    binds->property_type = m_ti;                                                                                                        \
    binds->method_set_##m_ts##_vec = &m_class::set_##m_prop;                                                                            \
    binds->method_get_##m_ts##_vec = &m_class::get_##m_prop;                                                                            \
    _prop_binds.push_back(binds);                                                                                                       \
    ByteProcessor bp = ByteProcessor<m_class>(binds, true);                                                                             \
    _byte_procs[_bound_slot] = bp;                                                                                                      \
}

#define PKT_SLOT(m_slot)                    _bound_slot = m_slot;
#define PKT_BIND_BYTE(m_class, m_prop)      _PKT_BIND_PROPERTY(m_class, m_prop, TYPE_BYTE, byte)
#define PKT_BIND_STR(m_class, m_prop)       _PKT_BIND_PROPERTY(m_class, m_prop, TYPE_STR, str)
#define PKT_BIND_POOLSTR(m_class, m_prop)   _PKT_BIND_PROPERTY(m_class, m_prop, TYPE_POOLSTR, poolstr)
#define PKT_BIND_BYTE_VEC(m_class, m_prop)  _PKT_BIND_PROPERTY_VEC(m_class, m_prop, TYPE_BYTE, byte)

#define PKT_BIND_I8(m_class, m_prop)        _PKT_BIND_PROPERTY( m_class, m_prop, TYPE_I8,  i8  )
#define PKT_BIND_I16(m_class, m_prop)       _PKT_BIND_PROPERTY( m_class, m_prop, TYPE_I16, i16 )
#define PKT_BIND_I32(m_class, m_prop)       _PKT_BIND_PROPERTY( m_class, m_prop, TYPE_I32, i32 )
#define PKT_BIND_I64(m_class, m_prop)       _PKT_BIND_PROPERTY( m_class, m_prop, TYPE_I64, i64 )

#define PKT_BIND_U8(m_class, m_prop)        _PKT_BIND_PROPERTY( m_class, m_prop, TYPE_U8,  u8  )
#define PKT_BIND_U16(m_class, m_prop)       _PKT_BIND_PROPERTY( m_class, m_prop, TYPE_U16, u16 )
#define PKT_BIND_U32(m_class, m_prop)       _PKT_BIND_PROPERTY( m_class, m_prop, TYPE_U32, u32 )
#define PKT_BIND_U64(m_class, m_prop)       _PKT_BIND_PROPERTY( m_class, m_prop, TYPE_U64, u64 )

#define PKT_BIND_F32(m_class, m_prop)       _PKT_BIND_PROPERTY( m_class, m_prop, TYPE_F32, f32 )
#define PKT_BIND_F64(m_class, m_prop)       _PKT_BIND_PROPERTY( m_class, m_prop, TYPE_F64, f64 )

#define PKT_BIND_I8_VEC(m_class, m_prop)    _PKT_BIND_PROPERTY_VEC( m_class, m_prop, TYPE_I8,  i8  )
#define PKT_BIND_I16_VEC(m_class, m_prop)   _PKT_BIND_PROPERTY_VEC( m_class, m_prop, TYPE_I16, i16 )
#define PKT_BIND_I32_VEC(m_class, m_prop)   _PKT_BIND_PROPERTY_VEC( m_class, m_prop, TYPE_I32, i32 )
#define PKT_BIND_I64_VEC(m_class, m_prop)   _PKT_BIND_PROPERTY_VEC( m_class, m_prop, TYPE_I64, i64 )
#define PKT_BIND_U8_VEC(m_class, m_prop)    _PKT_BIND_PROPERTY_VEC( m_class, m_prop, TYPE_U8,  u8  )
#define PKT_BIND_U16_VEC(m_class, m_prop)   _PKT_BIND_PROPERTY_VEC( m_class, m_prop, TYPE_U16, u16 )
#define PKT_BIND_U32_VEC(m_class, m_prop)   _PKT_BIND_PROPERTY_VEC( m_class, m_prop, TYPE_U32, u32 )
#define PKT_BIND_U64_VEC(m_class, m_prop)   _PKT_BIND_PROPERTY_VEC( m_class, m_prop, TYPE_U64, u64 )
#define PKT_BIND_F32_VEC(m_class, m_prop)   _PKT_BIND_PROPERTY_VEC( m_class, m_prop, TYPE_F32, f32 )
#define PKT_BIND_F64_VEC(m_class, m_prop)   _PKT_BIND_PROPERTY_VEC( m_class, m_prop, TYPE_F64, f64 )


template<class _Struct>
struct PropertyBinds {
    U8 property_type;
// setters
    void  (_Struct::     *method_set_byte) (const BYTE &);       // BYTE
    void  (_Struct::     *method_set_str)  (const STR &);          // STR
    void  (_Struct::     *method_set_i8)   (const I8 &);        // I8
    void  (_Struct::     *method_set_i16)  (const I16 &);         // I16
    void  (_Struct::     *method_set_i32)  (const I32 &);         // I32
    void  (_Struct::     *method_set_i64)  (const I64 &);         // I64
    void  (_Struct::     *method_set_u8)   (const U8 &);        //  U8
    void  (_Struct::     *method_set_u16)  (const U16 &);         // U16
    void  (_Struct::     *method_set_u32)  (const U32 &);         // U32
    void  (_Struct::     *method_set_u64)  (const U64 &);         // U64
    void  (_Struct::     *method_set_f32)  (const F32 &);       // F32
    void  (_Struct::     *method_set_f64)  (const F64 &);         // F64
// getters
    BYTE  (_Struct::     *method_get_byte) () const;     // BYTE
    STR   (_Struct::     *method_get_str)  () const;       // STR
    I8    (_Struct::     *method_get_i8)   () const;    // I8
    I16   (_Struct::     *method_get_i16)  () const;      // I16
    I32   (_Struct::     *method_get_i32)  () const;      // I32
    I64   (_Struct::     *method_get_i64)  () const;      // I64
    U8    (_Struct::     *method_get_u8)   () const;    // I8
    U16   (_Struct::     *method_get_u16)  () const;      // I16
    U32   (_Struct::     *method_get_u32)  () const;      // I32
    U64   (_Struct::     *method_get_u64)  () const;      // I64
    F32   (_Struct::     *method_get_f32)  () const;    // F32
    F64   (_Struct::     *method_get_f64)  () const;      // F64
// vec setters
    void  (_Struct::     *method_set_byte_vec) (const std::vector<BYTE> &);       // BYTE
    void  (_Struct::     *method_set_i8_vec) (const std::vector<I8> &);       // I8
    void  (_Struct::     *method_set_i16_vec) (const std::vector<I16> &);       // I16
    void  (_Struct::     *method_set_i32_vec) (const std::vector<I32> &);       // I32
    void  (_Struct::     *method_set_i64_vec) (const std::vector<I64> &);       // I64
    void  (_Struct::     *method_set_u8_vec) (const std::vector<U8> &);       // U8
    void  (_Struct::     *method_set_u16_vec) (const std::vector<U16> &);       // U16
    void  (_Struct::     *method_set_u32_vec) (const std::vector<U32> &);       // U32
    void  (_Struct::     *method_set_u64_vec) (const std::vector<U64> &);       // U64
    void  (_Struct::     *method_set_f32_vec) (const std::vector<F32> &);     // F32
    void  (_Struct::     *method_set_f64_vec) (const std::vector<F64> &);       // F64

// vec getters
    std::vector<BYTE>  *(_Struct::     *method_get_byte_vec) ();       // BYTE
    std::vector<I8>  *(_Struct::     *method_get_i8_vec) ();      // I8
    std::vector<I16>  *(_Struct::     *method_get_i16_vec) ();       // I16
    std::vector<I32>  *(_Struct::     *method_get_i32_vec) ();       // I32
    std::vector<I64>  *(_Struct::     *method_get_i64_vec) ();       // I64
    std::vector<U8>  *(_Struct::     *method_get_u8_vec) ();      // U8
    std::vector<U16>  *(_Struct::     *method_get_u16_vec) ();       // U16
    std::vector<U32>  *(_Struct::     *method_get_u32_vec) ();       // U32
    std::vector<U64>  *(_Struct::     *method_get_u64_vec) ();       // U64
    std::vector<F32>  *(_Struct::     *method_get_f32_vec) ();    // F32
    std::vector<F64>  *(_Struct::     *method_get_f64_vec) ();       // F64

// POOLSTR
    void      (_Struct::    *method_set_poolstr) (const POOLSTR &);
    POOLSTR * (_Struct::    *method_get_poolstr) ();
};

class PKTFormatConverter {
    
    typedef PKTByteVector::size_type size_type;

public:
    static void     pack_byte(const U8 &p_V, PKTByteVector *p_dest);
    static void     pack_str(const STR &p_V, PKTByteVector *p_dest);
    static void     pack_poolstr(POOLSTR *p_V, PKTByteVector *p_dest);

    static void     pack_i8(const I8 &p_V, PKTByteVector *p_dest);
    static void     pack_i16(const I16 &p_V, PKTByteVector *p_dest);
    static void     pack_i32(const I32 &p_V, PKTByteVector *p_dest);
    static void     pack_i64(const I64 &p_V, PKTByteVector *p_dest);

    static void     pack_u8(const U8 &p_V, PKTByteVector *p_dest);
    static void     pack_u16(const U16 &p_V, PKTByteVector *p_dest);
    static void     pack_u32(const U32 &p_V, PKTByteVector *p_dest);
    static void     pack_u64(const U64 &p_V, PKTByteVector *p_dest);

    static void     pack_f32(const F32 &p_V, PKTByteVector *p_dest);
    static void     pack_f64(const F64 &p_V, PKTByteVector *p_dest);

public:
    static BYTE     unpack_byte(PKTByteVector *p_data, const size_type &p_offset = 0);
    static STR      unpack_str(PKTByteVector *p_data, const size_type &p_len, const size_type &p_offset = 0);
    static POOLSTR  unpack_poolstr(PKTByteVector *p_data, const size_type &p_len, const size_type &p_offset = 0);

    static I8       unpack_i8(PKTByteVector *p_data, const size_type &p_offset = 0);
    static I16      unpack_i16(PKTByteVector *p_data, const size_type &p_offset = 0);
    static I32      unpack_i32(PKTByteVector *p_data, const size_type &p_offset = 0);
    static I64      unpack_i64(PKTByteVector *p_data, const size_type &p_offset = 0);

    static U8       unpack_u8(PKTByteVector *p_data, const size_type &p_offset = 0);
    static U16      unpack_u16(PKTByteVector *p_data, const size_type &p_offset = 0);
    static U32      unpack_u32(PKTByteVector *p_data, const size_type &p_offset = 0);
    static U64      unpack_u64(PKTByteVector *p_data, const size_type &p_offset = 0);

    static F32      unpack_f32(PKTByteVector *p_dest, const size_type &p_offset = 0);
    static F64      unpack_f64(PKTByteVector *p_dest, const size_type &p_offset = 0);
};


template<class _Struct>
class ByteProcessor {

private:
    bool _is_vec;
    PropertyBinds<_Struct> *_prop_binds;

public:
    inline ByteProcessor() : _prop_binds(NULL) { }
    inline ByteProcessor(PropertyBinds<_Struct> *p_binds_ptr, bool p_is_vec = false)
        : _prop_binds(p_binds_ptr), _is_vec(p_is_vec) { }

public:
    inline U8 get_property_type() const {
        if (!_prop_binds) {
            return 0;
        }
        return _prop_binds->property_type;
    }
    
    inline bool is_alloc_variant() const {
        if (!_prop_binds) {
            return false;
        }
        if (_is_vec) {
            return true;
        }
        switch (_prop_binds->property_type) {
            case TYPE_STR: {
                return true;
            } break;
            default: {
                return false;
            }
        }
    }

    inline bool is_vec() const {
        return _is_vec;
    }

    inline U16 get_vec_element_count(_Struct *p_obj) const {
        if (!_prop_binds) {
            return 0;
        }
        switch (_prop_binds->property_type) {
            case TYPE_BYTE: { return ((p_obj->*(_prop_binds->method_get_byte_vec))())->size(); } break;
            case TYPE_I8: { return ((p_obj->*(_prop_binds->method_get_i8_vec))())->size(); } break;
            case TYPE_I16: { return ((p_obj->*(_prop_binds->method_get_i16_vec))())->size(); } break;
            case TYPE_I32: { return ((p_obj->*(_prop_binds->method_get_i32_vec))())->size(); } break;
            case TYPE_I64: { return ((p_obj->*(_prop_binds->method_get_i64_vec))())->size(); } break;
            case TYPE_U8: { return ((p_obj->*(_prop_binds->method_get_u8_vec))())->size(); } break;
            case TYPE_U16: { return ((p_obj->*(_prop_binds->method_get_u16_vec))())->size(); } break;
            case TYPE_U32: { return ((p_obj->*(_prop_binds->method_get_u32_vec))())->size(); } break;
            case TYPE_U64: { return ((p_obj->*(_prop_binds->method_get_u64_vec))())->size(); } break;
            case TYPE_F32: { return ((p_obj->*(_prop_binds->method_get_f32_vec))())->size(); } break;
            case TYPE_F64: { return ((p_obj->*(_prop_binds->method_get_f64_vec))())->size(); } break;
            default: return 0;
        }
    }

    inline U16 get_alloc_size(_Struct *p_obj) const {
        if (!_prop_binds) {
            return 0;
        }
        switch (_prop_binds->property_type) {
            case TYPE_STR: {
                return (U16) (((p_obj->*(_prop_binds->method_get_str))()).size());
            } break;
            case TYPE_POOLSTR: {
                POOLSTR *_Ptr = (p_obj->*(_prop_binds->method_get_poolstr))();
                U16 _Strlen = 0;
                U16 _Pool_Size = (U16)_Ptr->size();
                if (!_Pool_Size)
                    return 0;
                U16 _Pad_Count = _Pool_Size - 1;
                for (U16 i = 0; i < _Pool_Size; i++) {
                    _Strlen += (*_Ptr)[i].size();
                }
                return (U16) (_Strlen + _Pad_Count);
            } break;
            case TYPE_BYTE: {
                return sizeof(char);
            } break;
            case TYPE_I8: {
                return sizeof(I8);
            } break;
            case TYPE_I16: {
                return sizeof(I16);
            } break;
            case TYPE_I32: {
                return sizeof(I32);
            } break;
            case TYPE_I64: {
                return sizeof(I64);
            } break;
            case TYPE_U8: {
                return sizeof(U8);
            } break;
            case TYPE_U16: {
                return sizeof(U16);
            } break;
            case TYPE_U32: {
                return sizeof(U32);
            } break;
            case TYPE_U64: {
                return sizeof(U64);
            } break;
            case TYPE_F32: {
                return sizeof(F32);
            }
            case TYPE_F64: {
                return sizeof(F64);
            }
            default: {
                return 0;
            }
        }
    }


#define _PKT_BYTEPROC_SERIALIZE_CASE_(m_lower, m_upper) {                                                                               \
    m_upper v = (m_upper) (p_obj->*(_prop_binds->method_get_##m_lower))();                                                              \
    PKTFormatConverter::pack_##m_lower(v, p_dest);                                                                                      \
} break;                                                                                                                                \

#define _PKT_BYTEPROC_DESERIALIZE_CASE_(m_lower, m_upper) {                                                                             \
    m_upper v = PKTFormatConverter::unpack_##m_lower(p_data, *p_byte_offset);                                                           \
    (p_dest_obj->*(_prop_binds->method_set_##m_lower))((m_upper) v);                                                                    \
    *p_byte_offset += sizeof(m_upper);                                                                                                  \
} break;


#define _PKT_BYTEPROC_SERIALIZE_VEC_CASE_(m_lower, m_upper) {                                                                           \
    std::vector<m_upper> *v_ptr = (p_obj->*(_prop_binds->method_get_##m_lower##_vec))();                                                \
    for (U16 i = 0; i < v_ptr->size(); i++)                                                                                             \
        PKTFormatConverter::pack_##m_lower(v_ptr->at(i), p_dest);                                                                       \
} break;                                                                                                                                \

#define _PKT_BYTEPROC_DESERIALIZE_VEC_CASE_(m_lower, m_upper) {                                                                         \
    std::vector<m_upper> v_vec;                                                                                                         \
    U16 elem_alloc_size = (U16) sizeof(m_upper);                                                                                        \
    U16 elem_count = p_var_len / elem_alloc_size;                                                                                       \
    for (U16 i = 0; i < elem_count; i++) {                                                                                              \
        m_upper v = PKTFormatConverter::unpack_##m_lower(p_data, *p_byte_offset);                                                       \
        v_vec.push_back(v);                                                                                                             \
        *p_byte_offset += elem_alloc_size;                                                                                              \
    }                                                                                                                                   \
    (p_dest_obj->*(_prop_binds->method_set_##m_lower##_vec))(v_vec);                                                                    \
} break;                                                                                                                                \


public:
    inline void serialize_property(_Struct *p_obj, PKTByteVector *p_dest) const {
        if (!_prop_binds) {
            return;
        }

        // ** TYPE VECTOR SERIALIZE **
        if (_is_vec) {
            switch (_prop_binds->property_type) {
                case TYPE_BYTE: _PKT_BYTEPROC_SERIALIZE_VEC_CASE_(byte, BYTE);
                case TYPE_I8: _PKT_BYTEPROC_SERIALIZE_VEC_CASE_(i8, I8);
                case TYPE_I16: _PKT_BYTEPROC_SERIALIZE_VEC_CASE_(i16, I16);
                case TYPE_I32: _PKT_BYTEPROC_SERIALIZE_VEC_CASE_(i32, I32);
                case TYPE_I64: _PKT_BYTEPROC_SERIALIZE_VEC_CASE_(i64, I64);
                case TYPE_U8: _PKT_BYTEPROC_SERIALIZE_VEC_CASE_(u8, U8);
                case TYPE_U16: _PKT_BYTEPROC_SERIALIZE_VEC_CASE_(u16, U16);
                case TYPE_U32: _PKT_BYTEPROC_SERIALIZE_VEC_CASE_(u32, U32);
                case TYPE_U64: _PKT_BYTEPROC_SERIALIZE_VEC_CASE_(u64, U64);
                case TYPE_F32: _PKT_BYTEPROC_SERIALIZE_VEC_CASE_(f32, F32);
                case TYPE_F64: _PKT_BYTEPROC_SERIALIZE_VEC_CASE_(f64, F64);
            }
            return;
        }

        switch (_prop_binds->property_type) {
            case TYPE_BYTE: _PKT_BYTEPROC_SERIALIZE_CASE_ (byte, BYTE);
            case TYPE_STR:  _PKT_BYTEPROC_SERIALIZE_CASE_ (str, STR);

            case TYPE_I8:   _PKT_BYTEPROC_SERIALIZE_CASE_ (i8,  I8);
            case TYPE_I16:  _PKT_BYTEPROC_SERIALIZE_CASE_ (i16, I16);
            case TYPE_I32:  _PKT_BYTEPROC_SERIALIZE_CASE_ (i32, I32);
            case TYPE_I64:  _PKT_BYTEPROC_SERIALIZE_CASE_ (i64, I64);

            case TYPE_U8:   _PKT_BYTEPROC_SERIALIZE_CASE_ (u8,  U8);
            case TYPE_U16:  _PKT_BYTEPROC_SERIALIZE_CASE_ (u16, U16);
            case TYPE_U32:  _PKT_BYTEPROC_SERIALIZE_CASE_ (u32, U32);
            case TYPE_U64:  _PKT_BYTEPROC_SERIALIZE_CASE_ (u64, U64);

            case TYPE_F32:  _PKT_BYTEPROC_SERIALIZE_CASE_ (f32, F32);
            case TYPE_F64:  _PKT_BYTEPROC_SERIALIZE_CASE_ (f64, F64);

            // POOLSTR needs custom case since the get method returns a pointer to the pool
            case TYPE_POOLSTR: {
                POOLSTR *v = (p_obj->*(_prop_binds->method_get_poolstr))();
                PKTFormatConverter::pack_poolstr(v, p_dest);
            } break;
        }
    }

    inline void deserialize_property(PKTByteVector *p_data, _Struct *p_dest_obj, U16 *p_byte_offset, const U16 &p_var_len) {
        if (!_prop_binds || p_byte_offset == NULL) {
            return;
        }

        // ** TYPE VECTOR DESERIALIZE **
        if (_is_vec) {
            switch (_prop_binds->property_type) {
                case TYPE_BYTE: _PKT_BYTEPROC_DESERIALIZE_VEC_CASE_(byte, BYTE);
                case TYPE_I8: _PKT_BYTEPROC_DESERIALIZE_VEC_CASE_(i8, I8);
                case TYPE_I16: _PKT_BYTEPROC_DESERIALIZE_VEC_CASE_(i16, I16);
                case TYPE_I32: _PKT_BYTEPROC_DESERIALIZE_VEC_CASE_(i32, I32);
                case TYPE_I64: _PKT_BYTEPROC_DESERIALIZE_VEC_CASE_(i64, I64);
                case TYPE_U8: _PKT_BYTEPROC_DESERIALIZE_VEC_CASE_(u8, U8);
                case TYPE_U16: _PKT_BYTEPROC_DESERIALIZE_VEC_CASE_(u16, U16);
                case TYPE_U32: _PKT_BYTEPROC_DESERIALIZE_VEC_CASE_(u32, U32);
                case TYPE_U64: _PKT_BYTEPROC_DESERIALIZE_VEC_CASE_(u64, U64);
                case TYPE_F32: _PKT_BYTEPROC_DESERIALIZE_VEC_CASE_(f32, F32);
                case TYPE_F64: _PKT_BYTEPROC_DESERIALIZE_VEC_CASE_(f64, F64);
            }
            return;
        }

        switch (_prop_binds->property_type) {
            case TYPE_BYTE: _PKT_BYTEPROC_DESERIALIZE_CASE_(byte, BYTE);

            case TYPE_I8: _PKT_BYTEPROC_DESERIALIZE_CASE_(i8, I8);
            case TYPE_I16: _PKT_BYTEPROC_DESERIALIZE_CASE_(i16, I16);
            case TYPE_I32: _PKT_BYTEPROC_DESERIALIZE_CASE_(i32, I32);
            case TYPE_I64: _PKT_BYTEPROC_DESERIALIZE_CASE_(i64, I64);

            case TYPE_U8: _PKT_BYTEPROC_DESERIALIZE_CASE_(u8, U8);
            case TYPE_U16: _PKT_BYTEPROC_DESERIALIZE_CASE_(u16, U16);
            case TYPE_U32: _PKT_BYTEPROC_DESERIALIZE_CASE_(u32, U32);
            case TYPE_U64: _PKT_BYTEPROC_DESERIALIZE_CASE_(u64, U64);
            case TYPE_F32: _PKT_BYTEPROC_DESERIALIZE_CASE_(f32, F32);
            case TYPE_F64: _PKT_BYTEPROC_DESERIALIZE_CASE_(f64, F64);

            case TYPE_STR: {
                STR v = PKTFormatConverter::unpack_str(p_data, p_var_len, *p_byte_offset);
                void (_Struct::*_set_str)(const STR &) = _prop_binds->method_set_str;
                (p_dest_obj->*_set_str)(v);
                *p_byte_offset += p_var_len;
            } break;

            case TYPE_POOLSTR: {
                POOLSTR v = PKTFormatConverter::unpack_poolstr(p_data, p_var_len, *p_byte_offset);
                void (_Struct::*_set_poolstr)(const POOLSTR &) = _prop_binds->method_set_poolstr;
                (p_dest_obj->*_set_poolstr)(v);
                *p_byte_offset += p_var_len;
            }
        }
    }
};

inline void PKTFormatConverter::pack_byte(const BYTE &p_V, PKTByteVector *p_dest) {
    p_dest->append(p_V);
}

inline void PKTFormatConverter::pack_str(const STR &p_V, PKTByteVector *p_dest) {
    U64 strlen = p_V.size();
    if (strlen == 0) {
        return;
    }
    U64 pos = p_dest->plus_alloc(strlen);
    for (int i = 0; i < strlen; i++) {
        (*p_dest)[pos + i] = (BYTE) p_V[i];
    }
}

inline void PKTFormatConverter::pack_poolstr(POOLSTR *p_V, PKTByteVector *p_dest) {
    U16 pool_size = (U16) p_V->size();
    if (pool_size == 0)
        return;

    for (U16 i = 0; i < pool_size; i++) {
        STR &elem = (*p_V)[i];
        if (elem.size() > 0) {
            PKTFormatConverter::pack_str(elem, p_dest);
        }
        if (i < (pool_size - 1)) {
            p_dest->append(0x0);    // terminator
        }
    }
}

inline void PKTFormatConverter::pack_i8(const I8 &p_V, PKTByteVector *p_dest) {
    p_dest->append(p_V);
}

inline void PKTFormatConverter::pack_i16(const I16 &p_V, PKTByteVector *p_dest) {
    U64 pos = p_dest->plus_alloc(2);
    (*p_dest)[pos+0] = (BYTE) ((p_V >> 8) & 0xff);
    (*p_dest)[pos+1] = (BYTE) ((p_V >> 0) & 0xff);
}

inline void PKTFormatConverter::pack_i32(const I32 &p_V, PKTByteVector *p_dest) {
    U64 pos = p_dest->plus_alloc(4);
    (*p_dest)[pos+0] = (BYTE) ((p_V >> 24) & 0xff);
    (*p_dest)[pos+1] = (BYTE) ((p_V >> 16) & 0xff);
    (*p_dest)[pos+2] = (BYTE) ((p_V >> 8) & 0xff);
    (*p_dest)[pos+3] = (BYTE) ((p_V >> 0) & 0xff);
}

inline void PKTFormatConverter::pack_i64(const I64 &p_V, PKTByteVector *p_dest) {
    U64 pos = p_dest->plus_alloc(8);
    (*p_dest)[pos+0] = (BYTE) ((p_V >> 56) & 0xff);
    (*p_dest)[pos+1] = (BYTE) ((p_V >> 48) & 0xff);
    (*p_dest)[pos+2] = (BYTE) ((p_V >> 40) & 0xff);
    (*p_dest)[pos+3] = (BYTE) ((p_V >> 32) & 0xff);
    (*p_dest)[pos+4] = (BYTE) ((p_V >> 24) & 0xff);
    (*p_dest)[pos+5] = (BYTE) ((p_V >> 16) & 0xff);
    (*p_dest)[pos+6] = (BYTE) ((p_V >> 8) & 0xff);
    (*p_dest)[pos+7] = (BYTE) ((p_V >> 0) & 0xff);
}

inline void PKTFormatConverter::pack_u8(const U8 &p_V, PKTByteVector *p_dest) {
    p_dest->append(p_V);
}

inline void PKTFormatConverter::pack_u16(const U16 &p_V, PKTByteVector *p_dest) {
    U64 pos = p_dest->plus_alloc(2);
    (*p_dest)[pos+0] = (BYTE) ((p_V >> 8) & 0xff);
    (*p_dest)[pos+1] = (BYTE) ((p_V >> 0) & 0xff);
}

inline void PKTFormatConverter::pack_u32(const U32 &p_V, PKTByteVector *p_dest) {
    U64 pos = p_dest->plus_alloc(4);
    (*p_dest)[pos+0] = (BYTE) ((p_V >> 24) & 0xff);
    (*p_dest)[pos+1] = (BYTE) ((p_V >> 16) & 0xff);
    (*p_dest)[pos+2] = (BYTE) ((p_V >> 8) & 0xff);
    (*p_dest)[pos+3] = (BYTE) ((p_V >> 0) & 0xff);
}

inline void PKTFormatConverter::pack_u64(const U64 &p_V, PKTByteVector *p_dest) {
    U64 pos = p_dest->plus_alloc(8);
    (*p_dest)[pos+0] = (BYTE) ((p_V >> 56) & 0xff);
    (*p_dest)[pos+1] = (BYTE) ((p_V >> 48) & 0xff);
    (*p_dest)[pos+2] = (BYTE) ((p_V >> 40) & 0xff);
    (*p_dest)[pos+3] = (BYTE) ((p_V >> 32) & 0xff);
    (*p_dest)[pos+4] = (BYTE) ((p_V >> 24) & 0xff);
    (*p_dest)[pos+5] = (BYTE) ((p_V >> 16) & 0xff);
    (*p_dest)[pos+6] = (BYTE) ((p_V >> 8) & 0xff);
    (*p_dest)[pos+7] = (BYTE) ((p_V >> 0) & 0xff);
}

inline void PKTFormatConverter::pack_f32(const F32 &p_V, PKTByteVector *p_dest) {
    return PKTFormatConverter::pack_u32(*(U32 *)(&p_V), p_dest);
}
inline void PKTFormatConverter::pack_f64(const F64 &p_V, PKTByteVector *p_dest) {
    return PKTFormatConverter::pack_u64(*(U64 *)(&p_V), p_dest);
}

inline BYTE PKTFormatConverter::unpack_byte(PKTByteVector *p_data, const U64 &p_offset) {
    return p_data->at(p_offset);
}

inline STR PKTFormatConverter::unpack_str(PKTByteVector *p_data, const U64 &p_len, const U64 &p_offset) {
    STR v;
    v.resize(p_len);
    for (int i = 0; i < p_len; i++) {
        v[i] = (char) p_data->at(p_offset + i);
    }
    return v;
}

inline POOLSTR PKTFormatConverter::unpack_poolstr(PKTByteVector *p_data, const U64 &p_len, const U64 &p_offset) {

    if (!p_len)
        return POOLSTR();

    POOLSTR vec;
    STR _elem;
    U16 _elem_idx = 0;
    for (int i = 0; i < p_len; i++) {
        const BYTE &b = p_data->at(p_offset + i);
        if (b != (BYTE) 0x0) {
            _elem.push_back((const char) b);
        }
        else {
            vec.push_back(_elem);
            _elem.clear();
        }
    }
    vec.push_back(_elem);
    return vec;
}

inline I8 PKTFormatConverter::unpack_i8(PKTByteVector *p_data, const U64 &p_offset) {
    return (I8) p_data->at(p_offset);
}

inline I16 PKTFormatConverter::unpack_i16(PKTByteVector *p_data, const U64 &p_offset) {
    I16 v = 0;
    v |= p_data->at(0 + p_offset) << 8;
    v |= p_data->at(1 + p_offset) << 0;
    return v;
}

inline I32 PKTFormatConverter::unpack_i32(PKTByteVector *p_data, const U64 &p_offset) {
    I32 v = 0;
    v |= p_data->at(0 + p_offset) << 24;
    v |= p_data->at(1 + p_offset) << 16;
    v |= p_data->at(2 + p_offset) << 8;
    v |= p_data->at(3 + p_offset) << 0;
    return v;
}

inline I64 PKTFormatConverter::unpack_i64(PKTByteVector *p_data, const U64 &p_offset) {
    I64 v = 0;
    v |= ((I64)p_data->at(0 + p_offset)) << 56;
    v |= ((I64)p_data->at(1 + p_offset)) << 48;
    v |= ((I64)p_data->at(2 + p_offset)) << 40;
    v |= ((I64)p_data->at(3 + p_offset)) << 32;
    v |= ((I64)p_data->at(4 + p_offset)) << 24;
    v |= ((I64)p_data->at(5 + p_offset)) << 16;
    v |= ((I64)p_data->at(6 + p_offset)) << 8;
    v |= ((I64)p_data->at(7 + p_offset)) << 0;
    return v;
}

inline U8 PKTFormatConverter::unpack_u8(PKTByteVector *p_data, const U64 &p_offset) {
    return p_data->at(p_offset);
}

inline U16 PKTFormatConverter::unpack_u16(PKTByteVector *p_data, const U64 &p_offset) {
    U16 v = 0;
    v |= p_data->at(0 + p_offset) << 8;
    v |= p_data->at(1 + p_offset) << 0;
    return v;
}

inline U32 PKTFormatConverter::unpack_u32(PKTByteVector *p_data, const U64 &p_offset) {
    U32 v = 0;
    v |= p_data->at(0 + p_offset) << 24;
    v |= p_data->at(1 + p_offset) << 16;
    v |= p_data->at(2 + p_offset) << 8;
    v |= p_data->at(3 + p_offset) << 0;
    return v;
}

inline U64 PKTFormatConverter::unpack_u64(PKTByteVector *p_data, const U64 &p_offset) {
    U64 v = 0;
    v |= ((U64)p_data->at(0 + p_offset)) << 56;
    v |= ((U64)p_data->at(1 + p_offset)) << 48;
    v |= ((U64)p_data->at(2 + p_offset)) << 40;
    v |= ((U64)p_data->at(3 + p_offset)) << 32;
    v |= ((U64)p_data->at(4 + p_offset)) << 24;
    v |= ((U64)p_data->at(5 + p_offset)) << 16;
    v |= ((U64)p_data->at(6 + p_offset)) << 8;
    v |= ((U64)p_data->at(7 + p_offset)) << 0;
    return v;
}

inline F32 PKTFormatConverter::unpack_f32(PKTByteVector *p_data, const U64 &p_offset) {
    U32 v = PKTFormatConverter::unpack_u32(p_data, p_offset);
    return *(F32 *)(&v);
}

inline F64 PKTFormatConverter::unpack_f64(PKTByteVector *p_data, const U64 &p_offset) {
    U64 v = PKTFormatConverter::unpack_u64(p_data, p_offset);
    return *(F64 *)(&v);
}