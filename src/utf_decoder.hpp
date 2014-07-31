#ifndef PUGIHTML_UTF_DECODER_HPP
#define PUGIHTML_UTF_DECODER_HPP 1


namespace pugihtml
{

template <typename Traits, typename opt_swap = opt_false>
struct utf_decoder {

	static inline typename Traits::value_type
	decode_utf8_block(const uint8_t* data, size_t size,
		typename Traits::value_type result)
	{
		const uint8_t utf8_byte_mask = 0x3f;

		while (size) {
			uint8_t lead = *data;

			// 0xxxxxxx -> U+0000..U+007F
			if (lead < 0x80) {
				result = Traits::low(result, lead);
				data += 1;
				size -= 1;

				// process aligned single-byte (ascii) blocks
				if ((reinterpret_cast<uintptr_t>(data) & 3) ==
					0) {
					while (size >= 4 && (*reinterpret_cast<const uint32_t*>(data) & 0x80808080) == 0) {
						result = Traits::low(result, data[0]);
						result = Traits::low(result, data[1]);
						result = Traits::low(result, data[2]);
						result = Traits::low(result, data[3]);
						data += 4;
						size -= 4;
					}
				}
			}
			// 110xxxxx -> U+0080..U+07FF
			else if ((unsigned)(lead - 0xC0) < 0x20 && size >= 2 && (data[1] & 0xc0) == 0x80)
			{
				result = Traits::low(result, ((lead & ~0xC0) << 6) | (data[1] & utf8_byte_mask));
				data += 2;
				size -= 2;
			}
			// 1110xxxx -> U+0800-U+FFFF
			else if ((unsigned)(lead - 0xE0) < 0x10 && size >= 3 && (data[1] & 0xc0) == 0x80 && (data[2] & 0xc0) == 0x80)
			{
				result = Traits::low(result, ((lead & ~0xE0) << 12) | ((data[1] & utf8_byte_mask) << 6) | (data[2] & utf8_byte_mask));
				data += 3;
				size -= 3;
			}
			// 11110xxx -> U+10000..U+10FFFF
			else if ((unsigned)(lead - 0xF0) < 0x08 && size >= 4 && (data[1] & 0xc0) == 0x80 && (data[2] & 0xc0) == 0x80 && (data[3] & 0xc0) == 0x80)
			{
				result = Traits::high(result, ((lead & ~0xF0) << 18) | ((data[1] & utf8_byte_mask) << 12) | ((data[2] & utf8_byte_mask) << 6) | (data[3] & utf8_byte_mask));
				data += 4;
				size -= 4;
			}
			// 10xxxxxx or 11111xxx -> invalid
			else
			{
				data += 1;
				size -= 1;
			}
		}

		return result;
	}

	static inline typename Traits::value_type decode_utf16_block(const uint16_t* data, size_t size, typename Traits::value_type result)
	{
		const uint16_t* end = data + size;

		while (data < end) {
			uint16_t lead = opt_swap::value ? endian_swap(*data) : *data;

			// U+0000..U+D7FF
			if (lead < 0xD800)
			{
				result = Traits::low(result, lead);
				data += 1;
			}
			// U+E000..U+FFFF
			else if ((unsigned)(lead - 0xE000) < 0x2000)
			{
				result = Traits::low(result, lead);
				data += 1;
			}
			// surrogate pair lead
			else if ((unsigned)(lead - 0xD800) < 0x400 && data + 1 < end)
			{
				uint16_t next = opt_swap::value ? endian_swap(data[1]) : data[1];

				if ((unsigned)(next - 0xDC00) < 0x400)
				{
					result = Traits::high(result, 0x10000 + ((lead & 0x3ff) << 10) + (next & 0x3ff));
					data += 2;
				}
				else
				{
					data += 1;
				}
			}
			else
			{
				data += 1;
			}
		}

		return result;
	}

	static inline typename Traits::value_type decode_utf32_block(const uint32_t* data, size_t size, typename Traits::value_type result)
	{
		const uint32_t* end = data + size;

		while (data < end)
		{
			uint32_t lead = opt_swap::value ? endian_swap(*data) : *data;

			// U+0000..U+FFFF
			if (lead < 0x10000)
			{
				result = Traits::low(result, lead);
				data += 1;
			}
			// U+10000..U+10FFFF
			else
			{
				result = Traits::high(result, lead);
				data += 1;
			}
		}

		return result;
	}
};

} // pugihtml.

#endif /* PUGIHTML_UTF_DECODER_HPP */
