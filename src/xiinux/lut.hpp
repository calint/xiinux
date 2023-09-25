#pragma once
//? replace use with std::unordered_map
namespace xiinux{template<class T>class lut final{
private:
	unsigned size;
	class el{
	public:
		char*key{nullptr};
		T data{nullptr};
		el*nxt{nullptr};
		inline el(char*key,T data):key{key},data{data}{}
		inline void delete_content_recurse(const bool delete_key){
			if(data)delete data;
			if(delete_key)delete[]key;
		}
	};
	el**array;
	// note. size must be 2^n because size-1 will be used for bitwise 'and'
	static inline unsigned hash(const char*key,const unsigned size){
		unsigned i=0;
		const char*p=key;
		while(*p)
			i+=(unsigned)*p++;
		i&=size-1;
		return i;
	}
public:
	// note. size must be 2^n because size-1 will be used for bitwise 'and'
	inline lut(const unsigned size=8):size(size){
		array=(el**)calloc(size_t(size),sizeof(el*));
	}
	inline~lut(){
		clear();
		free(array);
	}
	inline T operator[](const char*key){
		const unsigned h=hash(key,size);
		el*e=array[h];
		while(e){
			if(!strcmp(e->key,key))
				return e->data;
			e=e->nxt;
		}
		return nullptr;
	}
	inline void put(char*key,T data,bool allow_overwrite=true){
		const unsigned h=hash(key,size);
		el*e=array[h];
		if(!e){
			array[h]=new el(key,data);
			return;
		}
		while(e){
			if(!strcmp(e->key,key)){
				if(!allow_overwrite)
					throw"lut:put:overwrite";
				e->data=data;
				return;
			}
			if(!e->nxt){
				e->nxt=new el(key,data);
				return;
			}
			e=e->nxt;
		}
		throw"lut:put:unreachable";
	}
	inline void clear(){
		for(unsigned i=0;i<size;i++){
			el*e{array[i]};
			while(e){
				el*nxt{e->nxt};
				delete e;
				e=nxt;
			}
			array[i]=nullptr;
		}
	}
	inline void delete_content(const bool delete_keys){
		for(unsigned i=0;i<size;i++){
			el*e=array[i];
			while(e){
				el*nxt=e->nxt;
				e->delete_content_recurse(delete_keys);
				delete e;
				e=nxt;
			}
			array[i]=nullptr;
		}
	}
};}
