#ifndef lut_hpp
#define lut_hpp
namespace xiinux{
	template<class T>class lut{
	private:
		unsigned int size;
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
			inline void delete_content_recurse(bool delete_key){
	//			printf("delete lut data %s @ %p\n",typeid(*data).name(),(void*)this);
				if(data)
					delete data;
				if(delete_key)
					free(key);
				if(!nxt)
					return;
				nxt->delete_content_recurse(delete_key);
			}
		};
		el**array;
	public:
		static inline unsigned int hash(const char*key,const unsigned int roll){
			unsigned int i=0;
			const char*p=key;
			while(*p)
				i+=(unsigned int)*p++;
			i%=roll;
			return i;
		}
		inline lut(const unsigned int size=8):size(size){
	//		printf("new lut %p\n",(void*)this);
			array=(el**)calloc(size_t(size),sizeof(el*));
		}
		inline~lut(){
	//		printf("delete lut %p\n",(void*)this);
			clear();
			free(array);
		}
		inline T operator[](const char*key){
			const unsigned int h=hash(key,size);
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
			const unsigned int h=hash(key,size);
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
			for(unsigned int i=0;i<size;i++){
				el*e=array[i];
				if(!e)
					continue;
				delete e;
				array[i]=nullptr;
			}
		}
		inline void delete_content(const bool delete_keys){
	//		printf("delete lut content %p\n",(void*)this);
			for(unsigned int i=0;i<size;i++){
				el*e=array[i];
				if(!e)
					continue;
				e->delete_content_recurse(delete_keys);
				delete e;
				array[i]=nullptr;
			}
		}
	};
}
#endif
