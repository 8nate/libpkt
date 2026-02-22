
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

#define TYPE_BYTE                           0
#define TYPE_STR                            1

#define TYPE_I8                             2
#define TYPE_I16                            3
#define TYPE_I32                            4
#define TYPE_I64                            5

#define TYPE_U8                             6
#define TYPE_U16                            7
#define TYPE_U32                            8
#define TYPE_U64                            9

#define BYTE                                uint8_t     // 0
#define STR                                 std::string // 1

#define I8                                  int8_t      // 2
#define I16                                 int16_t     // 3
#define I32                                 int32_t     // 4
#define I64                                 int64_t     // 5

#define U8                                  uint8_t     // 6
#define U16                                 uint16_t    // 7
#define U32                                 uint32_t    // 8
#define U64                                 uint64_t    // 9


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
            if (proc.get_property_type() == TYPE_STR) {                                                                                 \
                U16 alloc_end = byte_offset + alloc_size - 1;                                                                           \
                PKTFormatConverter::pack_byte(slot, &_to);                                                                              \
                PKTFormatConverter::pack_u16(alloc_end, &_to);                                                                          \
            }                                                                                                                           \
            byte_offset += alloc_size;                                                                                                  \
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
        header_subdata.print_hex_table();                                                                                               \
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
            if (offsets.count(_pair.first))                                                                                             \
                var_len = offsets[_pair.first] - byte_offset + 1;                                                                       \
            ByteProcessor<m_class> &proc = _pair.second;                                                                                \
            proc.deserialize_property(&struct_data, p_obj, &byte_offset, var_len);                                                       \
        }\
    }                                                                                                                                   \
private: // reset back to private field
// PKT_STRUCT()

#define PKT_ALLOC_STATIC(m_class)                                                                                                       \
    class m_class;                                                                                                                      \
    U8 m_class::_bound_slot = 0;                                                                                                        \
    std::vector<PropertyBinds<m_class> *> m_class::_prop_binds = std::vector<PropertyBinds<m_class> *>();                               \
    std::map<U8, ByteProcessor<m_class>> m_class::_byte_procs = std::map<U8, ByteProcessor<m_class>>();                                 \

#define PKT_FREE_STATIC(m_class)        m_class::free_static()

#define PKT_CREATE_PROPERTY(m_Type, m_name)                                                                                             \
private:                                                                                                                                \
    m_Type _prop_##m_name;                                                                                                              \
public:                                                                                                                                 \
    inline void set_##m_name(const m_Type &p_V) { _prop_##m_name = p_V; }                                                               \
    inline m_Type get_##m_name() const { return _prop_##m_name; }                                                                       \
private:                                                                                                                                \



// internal bind macros
// #define _PKT_FORMAT_BIND                                __p_PKTSerialFormat_Ptr
#define _PKT_BIND_PTRS(m_c, m_p)                    &m_c::set_##m_p, &m_c::get_##m_p


// #define PKT_FORMAT_PTR                       PKTSerialFormat *_PKT_FORMAT_BIND

#define PKT_SLOT(m_slot)                _bound_slot = m_slot;


// byte
#define _PKT_BIND_PROPERTY(m_class, m_prop, m_ti, m_ts) {                                                                               \
    PropertyBinds<m_class> *binds = new PropertyBinds<m_class>;                                                                         \
    binds->property_type = m_ti;                                                                                                        \
    binds->method_set_##m_ts = &m_class::set_##m_prop;                                                                                  \
    binds->method_get_##m_ts = &m_class::get_##m_prop;                                                                                  \
    _prop_binds.push_back(binds);                                                                                                       \
    ByteProcessor bp = ByteProcessor<m_class>(binds);                                                                                   \
    _byte_procs[_bound_slot] = bp;                                                                                                      \
}

#define PKT_BIND_BYTE(m_class, m_prop)      _PKT_BIND_PROPERTY(m_class, m_prop, TYPE_BYTE, byte)
#define PKT_BIND_STR(m_class, m_prop)       _PKT_BIND_PROPERTY(m_class, m_prop, TYPE_STR, str)

#define PKT_BIND_I8(m_class, m_prop)        _PKT_BIND_PROPERTY( m_class, m_prop, TYPE_I8,  i8  )
#define PKT_BIND_I16(m_class, m_prop)       _PKT_BIND_PROPERTY( m_class, m_prop, TYPE_I16, i16 )
#define PKT_BIND_I32(m_class, m_prop)       _PKT_BIND_PROPERTY( m_class, m_prop, TYPE_I32, i32 )
#define PKT_BIND_I64(m_class, m_prop)       _PKT_BIND_PROPERTY( m_class, m_prop, TYPE_I64, i64 )

#define PKT_BIND_U8(m_class, m_prop)        _PKT_BIND_PROPERTY( m_class, m_prop, TYPE_U8,  u8  )
#define PKT_BIND_U16(m_class, m_prop)       _PKT_BIND_PROPERTY( m_class, m_prop, TYPE_U16, u16 )
#define PKT_BIND_U32(m_class, m_prop)       _PKT_BIND_PROPERTY( m_class, m_prop, TYPE_U32, u32 )
#define PKT_BIND_U64(m_class, m_prop)       _PKT_BIND_PROPERTY( m_class, m_prop, TYPE_U64, u64 )


#define VALIDATE_SUB_RANGE(x, y, max)       ((y > x) && (x >= 0 && x <= max) && (y > 0 && y <= max))

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
    inline size_type plus_alloc(size_type p_count) {
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
            if (_El == p_Val)
                return _Idx;
            _Idx++;
        }
        return -1;
    }

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

public:
    inline void print_hex_table() const {
        for (const BYTE &_El : _data) {
            printf("0x%X ", _El);
        }
        printf("\n");
    }
};     


class PKTFormatConverter {
    
public:
    static void pack_byte(const U8 &p_V, PKTByteVector *p_dest);
    static void pack_str(const STR &p_V, PKTByteVector *p_dest);

    static void pack_i8(const I8 &p_V, PKTByteVector *p_dest);
    static void pack_i16(const I16 &p_V, PKTByteVector *p_dest);
    static void pack_i32(const I32 &p_V, PKTByteVector *p_dest);
    static void pack_i64(const I64 &p_V, PKTByteVector *p_dest);

    static void pack_u8(const U8 &p_V, PKTByteVector *p_dest);
    static void pack_u16(const U16 &p_V, PKTByteVector *p_dest);
    static void pack_u32(const U32 &p_V, PKTByteVector *p_dest);
    static void pack_u64(const U64 &p_V, PKTByteVector *p_dest);

public:
    static BYTE unpack_byte(PKTByteVector *p_data, const PKTByteVector::size_type &p_offset = 0);
    static STR  unpack_str(PKTByteVector *p_data, const PKTByteVector::size_type &p_len, const PKTByteVector::size_type &p_offset = 0);

    static I8   unpack_i8(PKTByteVector *p_data, const PKTByteVector::size_type &p_offset = 0);
    static I16  unpack_i16(PKTByteVector *p_data, const PKTByteVector::size_type &p_offset = 0);
    static I32  unpack_i32(PKTByteVector *p_data, const PKTByteVector::size_type &p_offset = 0);
    static I64  unpack_i64(PKTByteVector *p_data, const PKTByteVector::size_type &p_offset = 0);

    static U8   unpack_u8(PKTByteVector *p_data, const PKTByteVector::size_type &p_offset = 0);
    static U16  unpack_u16(PKTByteVector *p_data, const PKTByteVector::size_type &p_offset = 0);
    static U32  unpack_u32(PKTByteVector *p_data, const PKTByteVector::size_type &p_offset = 0);
    static U64  unpack_u64(PKTByteVector *p_data, const PKTByteVector::size_type &p_offset = 0);
};


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
};


template<class _Struct>
class ByteProcessor {

private:
    PropertyBinds<_Struct> *_prop_binds;

public:
    inline ByteProcessor() : _prop_binds(NULL) { }
    inline ByteProcessor(PropertyBinds<_Struct> *p_binds_ptr) : _prop_binds(p_binds_ptr) { }

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
        switch (_prop_binds->property_type) {
            case TYPE_STR: {
                return true;
            } break;
            default: {
                return false;
            }
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

public:
    inline void serialize_property(_Struct *p_obj, PKTByteVector *p_dest) const {
        if (!_prop_binds) {
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
        }
    }

    inline void deserialize_property(PKTByteVector *p_data, _Struct *p_dest_obj, U16 *p_byte_offset, const U16 &p_var_len) {
        if (!_prop_binds || p_byte_offset == NULL) {
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

            case TYPE_STR: {
                STR v = PKTFormatConverter::unpack_str(p_data, p_var_len, *p_byte_offset);
                void (_Struct::*_method_set_str)(const STR &) = _prop_binds->method_set_str;
                (p_dest_obj->*_method_set_str)(v);
                *p_byte_offset += p_var_len;
            } break;
        }
    }
};


class PKTStructure {

public:

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


inline BYTE PKTFormatConverter::unpack_byte(PKTByteVector *p_data, const U64 &p_offset) {
    BYTE v = 0;
    v |= p_data->at(p_offset + 0) << 0;
    return (BYTE)v;
}


inline STR PKTFormatConverter::unpack_str(PKTByteVector *p_data, const U64 &p_len, const U64 &p_offset) {
    STR v;
    v.resize(p_len);
    for (int i = 0; i < p_len; i++) {
        v[i] = (char) p_data->at(p_offset + i);
    }
    return v;
}

inline I8 PKTFormatConverter::unpack_i8(PKTByteVector *p_data, const U64 &p_offset) {
    return p_data->at(p_offset);
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