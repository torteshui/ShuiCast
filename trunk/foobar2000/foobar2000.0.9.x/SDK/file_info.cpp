#include "foobar2000.h"

t_size file_info::meta_find_ex(const char * p_name,t_size p_name_length) const
{
	t_size n, m = meta_get_count();
	for(n=0;n<m;n++)
	{
		if (!stricmp_utf8_ex(meta_enum_name(n),infinite,p_name,p_name_length)) return n;
	}
	return infinite;
}

bool file_info::meta_exists_ex(const char * p_name,t_size p_name_length) const
{
	return meta_find_ex(p_name,p_name_length) != infinite;
}

void file_info::meta_remove_field_ex(const char * p_name,t_size p_name_length)
{
	t_size index = meta_find_ex(p_name,p_name_length);
	if (index!=infinite) meta_remove_index(index);
}


void file_info::meta_remove_index(t_size p_index)
{
	meta_remove_mask(bit_array_one(p_index));
}

void file_info::meta_remove_all()
{
	meta_remove_mask(bit_array_true());
}

void file_info::meta_remove_value(t_size p_index,t_size p_value)
{
	meta_remove_values(p_index,bit_array_one(p_value));
}

t_size file_info::meta_get_count_by_name_ex(const char * p_name,t_size p_name_length) const
{
	t_size index = meta_find_ex(p_name,p_name_length);
	if (index == infinite) return 0;
	return meta_enum_value_count(index);
}

t_size file_info::info_find_ex(const char * p_name,t_size p_name_length) const
{
	t_size n, m = info_get_count();
	for(n=0;n<m;n++)
	{
		if (!stricmp_utf8_ex(info_enum_name(n),infinite,p_name,p_name_length)) return n;
	}
	return infinite;
}

bool file_info::info_exists_ex(const char * p_name,t_size p_name_length) const
{
	return info_find_ex(p_name,p_name_length) != infinite;
}

void file_info::info_remove_index(t_size p_index)
{
	info_remove_mask(bit_array_one(p_index));
}

void file_info::info_remove_all()
{
	info_remove_mask(bit_array_true());
}

bool file_info::info_remove_ex(const char * p_name,t_size p_name_length)
{
	t_size index = info_find_ex(p_name,p_name_length);
	if (index != infinite)
	{
		info_remove_index(index);
		return true;
	}
	else return false;
}

void file_info::copy_meta_single(const file_info & p_source,t_size p_index)
{
	copy_meta_single_rename(p_source,p_index,p_source.meta_enum_name(p_index));
}

void file_info::copy_meta_single_nocheck(const file_info & p_source,t_size p_index)
{
	const char * name = p_source.meta_enum_name(p_index);
	t_size n, m = p_source.meta_enum_value_count(p_index);
	t_size new_index = infinite;
	for(n=0;n<m;n++)
	{
		const char * value = p_source.meta_enum_value(p_index,n);
		if (n == 0) new_index = meta_set_nocheck(name,value);
		else meta_add_value(new_index,value);
	}
}

void file_info::copy_meta_single_by_name_ex(const file_info & p_source,const char * p_name,t_size p_name_length)
{
	t_size index = p_source.meta_find_ex(p_name,p_name_length);
	if (index != infinite) copy_meta_single(p_source,index);
}

void file_info::copy_info_single_by_name_ex(const file_info & p_source,const char * p_name,t_size p_name_length)
{
	t_size index = p_source.info_find_ex(p_name,p_name_length);
	if (index != infinite) copy_info_single(p_source,index);
}

void file_info::copy_meta_single_by_name_nocheck_ex(const file_info & p_source,const char * p_name,t_size p_name_length)
{
	t_size index = p_source.meta_find_ex(p_name,p_name_length);
	if (index != infinite) copy_meta_single_nocheck(p_source,index);
}

void file_info::copy_info_single_by_name_nocheck_ex(const file_info & p_source,const char * p_name,t_size p_name_length)
{
	t_size index = p_source.info_find_ex(p_name,p_name_length);
	if (index != infinite) copy_info_single_nocheck(p_source,index);
}

void file_info::copy_info_single(const file_info & p_source,t_size p_index)
{
	info_set(p_source.info_enum_name(p_index),p_source.info_enum_value(p_index));
}

void file_info::copy_info_single_nocheck(const file_info & p_source,t_size p_index)
{
	info_set_nocheck(p_source.info_enum_name(p_index),p_source.info_enum_value(p_index));
}

void file_info::copy_meta(const file_info & p_source)
{
	if (&p_source != this) {
		meta_remove_all();
		t_size n, m = p_source.meta_get_count();
		for(n=0;n<m;n++)
			copy_meta_single_nocheck(p_source,n);
	}
}

void file_info::copy_info(const file_info & p_source)
{
	if (&p_source != this) {
		info_remove_all();
		t_size n, m = p_source.info_get_count();
		for(n=0;n<m;n++)
			copy_info_single_nocheck(p_source,n);
	}
}

void file_info::copy(const file_info & p_source)
{
	if (&p_source != this) {
		copy_meta(p_source);
		copy_info(p_source);
		set_length(p_source.get_length());
		set_replaygain(p_source.get_replaygain());
	}
}


const char * file_info::meta_get_ex(const char * p_name,t_size p_name_length,t_size p_index) const
{
	t_size index = meta_find_ex(p_name,p_name_length);
	if (index == infinite) return 0;
	t_size max = meta_enum_value_count(index);
	if (p_index >= max) return 0;
	return meta_enum_value(index,p_index);
}

const char * file_info::info_get_ex(const char * p_name,t_size p_name_length) const
{
	t_size index = info_find_ex(p_name,p_name_length);
	if (index == infinite) return 0;
	return info_enum_value(index);
}

t_int64 file_info::info_get_int(const char * name) const
{
	assert(pfc::is_valid_utf8(name));
	const char * val = info_get(name);
	if (val==0) return 0;
	return _atoi64(val);
}

t_int64 file_info::info_get_length_samples() const
{
	t_int64 ret = 0;
	double len = get_length();
	t_int64 srate = info_get_int("samplerate");

	if (srate>0 && len>0)
	{
		ret = audio_math::time_to_samples(len,(unsigned)srate);
	}
	return ret;
}

double file_info::info_get_float(const char * name) const
{
	const char * ptr = info_get(name);
	if (ptr) return pfc::string_to_float(ptr);
	else return 0;
}

void file_info::info_set_int(const char * name,t_int64 value)
{
	assert(pfc::is_valid_utf8(name));
	char temp[32];
	_i64toa(value,temp,10);
	info_set(name,temp);
}

void file_info::info_set_float(const char * name,double value,unsigned precision,bool force_sign,const char * unit)
{
	assert(pfc::is_valid_utf8(name));
	assert(unit==0 || strlen(unit) <= 64);
	char temp[128];
	pfc::float_to_string(temp,64,value,precision,force_sign);
	temp[63] = 0;
	if (unit)
	{
		strcat(temp," ");
		strcat(temp,unit);
	}
	info_set(name,temp);
}


void file_info::info_set_replaygain_album_gain(float value)
{
	replaygain_info temp = get_replaygain();
	temp.m_album_gain = value;
	set_replaygain(temp);
}

void file_info::info_set_replaygain_album_peak(float value)
{
	replaygain_info temp = get_replaygain();
	temp.m_album_peak = value;
	set_replaygain(temp);
}

void file_info::info_set_replaygain_track_gain(float value)
{
	replaygain_info temp = get_replaygain();
	temp.m_track_gain = value;
	set_replaygain(temp);
}

void file_info::info_set_replaygain_track_peak(float value)
{
	replaygain_info temp = get_replaygain();
	temp.m_track_peak = value;
	set_replaygain(temp);
}


static bool is_valid_bps(t_int64 val)
{
	return val>0 && val<=256;
}

unsigned file_info::info_get_decoded_bps() const
{
	t_int64 val = info_get_int("decoded_bitspersample");
	if (is_valid_bps(val)) return (unsigned)val;
	val = info_get_int("bitspersample");
	if (is_valid_bps(val)) return (unsigned)val;
	return 0;

}

void file_info::reset()
{
	info_remove_all();
	meta_remove_all();
	set_length(0);
	reset_replaygain();
}

void file_info::reset_replaygain()
{
	replaygain_info temp;
	temp.reset();
	set_replaygain(temp);
}

void file_info::copy_meta_single_rename_ex(const file_info & p_source,t_size p_index,const char * p_new_name,t_size p_new_name_length)
{
	t_size n, m = p_source.meta_enum_value_count(p_index);
	t_size new_index = infinite;
	for(n=0;n<m;n++)
	{
		const char * value = p_source.meta_enum_value(p_index,n);
		if (n == 0) new_index = meta_set_ex(p_new_name,p_new_name_length,value,infinite);
		else meta_add_value(new_index,value);
	}
}

t_size file_info::meta_add_ex(const char * p_name,t_size p_name_length,const char * p_value,t_size p_value_length)
{
	t_size index = meta_find_ex(p_name,p_name_length);
	if (index == infinite) return meta_set_nocheck_ex(p_name,p_name_length,p_value,p_value_length);
	else
	{
		meta_add_value_ex(index,p_value,p_value_length);
		return index;
	}
}

void file_info::meta_add_value_ex(t_size p_index,const char * p_value,t_size p_value_length)
{
	meta_insert_value_ex(p_index,meta_enum_value_count(p_index),p_value,p_value_length);
}


t_size file_info::meta_calc_total_value_count() const
{
	t_size n, m = meta_get_count(), ret = 0;
	for(n=0;n<m;n++) ret += meta_enum_value_count(n);
	return ret;
}

bool file_info::info_set_replaygain_ex(const char * p_name,t_size p_name_len,const char * p_value,t_size p_value_len)
{
	replaygain_info temp = get_replaygain();
	if (temp.set_from_meta_ex(p_name,p_name_len,p_value,p_value_len))
	{
		set_replaygain(temp);
		return true;
	}
	else return false;
}

void file_info::info_set_replaygain_auto_ex(const char * p_name,t_size p_name_len,const char * p_value,t_size p_value_len)
{
	if (!info_set_replaygain_ex(p_name,p_name_len,p_value,p_value_len))
		info_set_ex(p_name,p_name_len,p_value,p_value_len);
}

bool replaygain_info::g_equal(const replaygain_info & item1,const replaygain_info & item2)
{
	return	item1.m_album_gain == item2.m_album_gain &&
			item1.m_track_gain == item2.m_track_gain &&
			item1.m_album_peak == item2.m_album_peak &&
			item1.m_track_peak == item2.m_track_peak;
}

bool file_info::are_meta_fields_identical(t_size p_index1,t_size p_index2) const
{
	const t_size count = meta_enum_value_count(p_index1);
	if (count != meta_enum_value_count(p_index2)) return false;
	t_size n;
	for(n=0;n<count;n++)
	{
		if (strcmp(meta_enum_value(p_index1,n),meta_enum_value(p_index2,n))) return false;
	}
	return true;
}


bool file_info::meta_format(const char * p_name,pfc::string_base & p_out) const
{
	p_out.reset();
	t_size index = meta_find(p_name);
	if (index == infinite) return false;
	t_size val, count = meta_enum_value_count(index);
	if (count == 0) return false;
	for(val=0;val<count;val++)
	{
		if (val > 0) p_out += ", ";
		p_out += meta_enum_value(index,val);
	}
	return true;
}

void file_info::info_calculate_bitrate(t_filesize p_filesize,double p_length)
{
	info_set_bitrate((unsigned)floor((double)p_filesize * 8 / (p_length * 1000) + 0.5));
}

bool file_info::is_encoding_lossy() const {
	const char * encoding = info_get("encoding");
	if (encoding != NULL) {
		if (stricmp_utf8(encoding,"lossy") == 0 /*|| stricmp_utf8(encoding,"hybrid") == 0*/) return true;
	} else {
		//the old way
		if (info_get("bitspersample") == NULL) return true;
	}
	return false;
}

bool file_info::g_is_meta_equal(const file_info & p_item1,const file_info & p_item2) {
	t_size count = p_item1.meta_get_count();
	if (count != p_item2.meta_get_count()) return false;
	for(t_size n1=0; n1<count; n1++) {
		t_size n2 = p_item2.meta_find(p_item1.meta_enum_name(n1));
		if (n2 == infinite) return false;
		t_size value_count = p_item1.meta_enum_value_count(n1);
		if (value_count != p_item2.meta_enum_value_count(n2)) return false;
		for(t_size v = 0; v < value_count; v++) {
			if (strcmp(p_item1.meta_enum_value(n1,v),p_item2.meta_enum_value(n2,v)) != 0) return false;
		}
	}
	return true;
}

bool file_info::g_is_info_equal(const file_info & p_item1,const file_info & p_item2) {
	t_size count = p_item1.info_get_count();
	if (count != p_item2.info_get_count()) return false;
	for(t_size n1=0; n1<count; n1++) {
		t_size n2 = p_item2.info_find(p_item1.info_enum_name(n1));
		if (n2 == infinite) return false;
		if (strcmp(p_item1.info_enum_value(n1),p_item2.info_enum_value(n2)) != 0) return false;
	}
	return true;
}