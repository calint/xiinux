#ifndef lst_hpp
#define lst_hpp
namespace xiinux{
	template<class T>class lst{
	private:
		class el{
		public:
			T ptr{0};
			el*nxt{nullptr};
			inline el(T ptr):ptr{ptr}{}
		};
		el*first{nullptr};
		el*last{nullptr};
		size_t size{0};
		inline void clr(){
			el*e=first;
			while(e){
				el*ee=e;
				delete ee;
				e=e->nxt;
			}
		}
	public:
		inline lst(){}
		inline~lst(){clr();}
		inline void clear(){clr();first=last=nullptr;}
		inline void to(FILE*f)const{
			fprintf(f,"[first=%p;last=%p][",first,last);
			el*e=first;
			while(e){
				fprintf(f,"%p",e);
				e=e->nxt;
				if(e)
					fprintf(f," ");
			}
			fprintf(f,"]\n");
		}
		inline void add(T ptr){
			size++;
			el*e=new el(ptr);
			if(!last){
				first=last=e;
				return;
			}
			last=last->nxt=e;
		}
		inline void addfirst(T ptr){
			size++;
			el*e=new el(ptr);
			if(!first){
				first=last=e;
				return;
			}
			e->nxt=first;
			first=e;
		}
		inline T take_first(){
			if(first==nullptr)
				return nullptr;
			size--;
			T ret=first->ptr;
			if(!first->nxt){
				delete first;
				first=last=nullptr;
				return ret;
			}
			el*e=first;
			first=first->nxt;
			delete e;
			return ret;
		}
		inline T find(T key){
			el*e=first;
			while(e){
				if(e->ptr==key)
					return e->ptr;
				e=e->nxt;
			}
			return nullptr;
		}
		inline size_t getsize()const{return size;}
		inline bool isempty()const{return size==0;}
		inline void foreach(bool f(T)){
			if(!first)
				return;
			el*e=first;
			while(e){
				if(!f(e->ptr))
					break;
				e=e->nxt;
			}
		}
		inline void foreach2(const std::function<bool (T)>&f){
			if(!first)
				return;
			el*e=first;
			while(e){
				if(!f(e->ptr))
					break;
				e=e->nxt;
			}
		}
	};
}
#endif
