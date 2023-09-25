#pragma once
//? replace use with std::unordered_map
namespace xiinux{template<class T>class lut{
private:
	unsigned size;
	class el{
	public:
		char*key{nullptr};
		T data{nullptr};
		el*nxt{nullptr};
		inline el(char*key,T data):key{key},data{data}{
//			printf(" * new lut element %s @ %p\n",key,(void*)this);
		}
		inline~el(){
//			printf("delete lut element %s  %s @ %p\n",key,typeid(*this).name(),(void*)this);
//			printf("delete lut element %s  @ %p\n",key,(void*)this);
//			printf("delete lut element @ %p\n",(void*)this);
			if(nxt)
				delete nxt;
		}
		// todo: rewrite without recursion
		inline void delete_content_recurse(const bool delete_key){
//			printf("delete lut data %s @ %p\n",typeid(*data).name(),(void*)this);
			if(data)
				delete data;
			if(delete_key)
				delete[]key;
			if(!nxt)
				return;
			nxt->delete_content_recurse(delete_key);
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
//		printf("new lut %p\n",(void*)this);
		array=(el**)calloc(size_t(size),sizeof(el*));
	}
	inline~lut(){
//		printf("delete lut %p\n",(void*)this);
		clear();
		free(array);
	}
	inline T operator[](const char*key){
		const unsigned h=hash(key,size);
		el*e=array[h];
		if(!e)
			return nullptr;
		while(true){
			if(!strcmp(e->key,key)){
				return e->data;
			}
			if(e->nxt){
				e=e->nxt;
				continue;
			}
			return nullptr;
		}
	}
	inline void put(char*key,T data,bool allow_overwrite=true){
		const unsigned h=hash(key,size);
		el*l=array[h];
		if(!l){
			array[h]=new el(key,data);
			return;
		}
		while(1){
			if(!strcmp(l->key,key)){
				if(!allow_overwrite)
					throw"lutoverwrite";
				l->data=data;
				return;
			}
			if(l->nxt){
				l=l->nxt;
				continue;
			}
			l->nxt=new el(key,data);
			return;
		}
	}
	inline void clear(){
//		printf("clear lut %p\n",(void*)this);
		for(unsigned i=0;i<size;i++){
			el*e=array[i];
			if(!e)
				continue;
			delete e;
			array[i]=nullptr;
		}
	}
	inline void delete_content(const bool delete_keys){
//		printf("delete lut content %p\n",(void*)this);
		for(unsigned i=0;i<size;i++){
			el*e=array[i];
			if(!e)
				continue;
			e->delete_content_recurse(delete_keys);
			delete e;
			array[i]=nullptr;
		}
	}
};}
