namespace bustub{
	struct TableInfo{
		page_id_t tableFirstPageId;
		page_id_t indexRootPageId;			
	};
    
    class Table{
    public:
        Table(bool isCreate, const std::string &indexPath, const std::string &tablePath,
        const std::vector<std::tuple<std::string, int, int>>& tableInfos, size_t bpmSize = 10000){
            printf("table begin\n");
            tableDiskManager_ = new DiskManager(tablePath);
            indexDiskManager_ = new DiskManager(indexPath);
            indexBPM_ = new BufferPoolManager(bpmSize, indexDiskManager_);
            tableBPM_ = new BufferPoolManager(bpmSize, tableDiskManager_);

            //GenericKey<64> index_key;
            std::vector<Column> cols;
            Column col("key", TypeId::BIGINT);
            cols.push_back(col);
            Schema *indexSchema = new Schema(cols);
            GenericComparator<64> comparator(indexSchema);
            std::string indexName = "indexName";
            tree_ = new BPlusTree<GenericKey<64>, RID, GenericComparator<64>>(indexName, indexBPM_, comparator, 4, 4);
           
            
            //make table schema

        
            cols.clear();
            for(const auto &tableInfo : tableInfos){
                TypeId typeId = GetTypeId(std::get<1>(tableInfo));
                if(typeId == TypeId::VARCHAR){
                    cols.push_back({std::get<0>(tableInfo), typeId, std::get<2>(tableInfo)});
                }else{
                    cols.push_back({std::get<0>(tableInfo), typeId});
                }
            }
            Schema tableSchema(cols);
            tableSchema_ = new Schema(cols);
            

            if(isCreate){
                page_id_t pageId;
                auto tableMetadataPage = tableBPM_->NewPage(&pageId);
                
                tableHeap_ = new TableHeap(tableBPM_);
                TableInfo *tableInfo = reinterpret_cast<TableInfo*>(tableMetadataPage);
                tableInfo->tableFirstPageId = tableHeap_->GetFirstPageId();
                tableInfo->indexRootPageId = tree_->GetRootPageId();
                indexRootPageId_ = tree_->GetRootPageId();
                tableBPM_->UnpinPage(pageId, true);
                printf("page root id %d\n", tree_->GetRootPageId());
                printf("table first id %d\n", tableHeap_->GetFirstPageId());
                tableBPM_->FlushAllPages();
                indexBPM_->FlushAllPages();   
            }else{
                auto tableMetadataPage = tableBPM_->FetchPage(0);
                tableBPM_->UnpinPage(0, false);
                TableInfo *tableInfo = reinterpret_cast<TableInfo*>(tableMetadataPage);
                tree_->SetRootPageId(tableInfo->indexRootPageId);
                tableHeap_ = new TableHeap(tableBPM_, tableInfo->tableFirstPageId);
                printf("indexRootPageId :%d, tableFirstPageid :%d\n", 
                tableInfo->indexRootPageId, tableInfo->tableFirstPageId);
                
            }
            
        }

        void ChangeIndexRootPageId(page_id_t newRootPageId){
            indexRootPageId_ = newRootPageId;
            auto tableMetadataPage = tableBPM_->FetchPage(0);
            TableInfo *tableInfo = reinterpret_cast<TableInfo*>(tableMetadataPage);
            tableInfo->indexRootPageId = indexRootPageId_;
            tableBPM_->UnpinPage(0, true);
        }
        TypeId GetTypeId(int id){
            TypeId typeId;
            switch (id)
                {
                case 1:
                    typeId = TypeId::INTEGER;
                    break;
                case 2:
                    typeId = TypeId::BIGINT;
                    break;
                case 3:
                    typeId = TypeId::VARCHAR;
                    break;
                default:
                    typeId = TypeId::INVALID;
                }
            return typeId;

        }
        bool Insert(const std::vector<std::string> &colValues){
            GenericKey<64> index_key;
            RID rid;
            auto cols = tableSchema_->GetColumns();
            int n = colValues.size();
            std::cout<<"n="<<n<<", cols size:"<<cols.size()<<"\n";
            assert(n == cols.size());
            
            std::vector<Value> values;
            values.reserve(n);
            
            for(int i = 0; i < n; i++){
                TypeId typeId = cols[i].GetType();
                switch (typeId)
                {
                case TypeId::INTEGER:
                    values.push_back({TypeId::INTEGER, stoi(colValues[i])});  
                    break;
                case TypeId::BIGINT:
                    values.push_back({TypeId::BIGINT, stoul(colValues[i])});  
                    break;
                case TypeId::VARCHAR:
                    values.push_back({TypeId::VARCHAR, colValues[i]});   
                    break;
                default:
                    break;
                }
            }
            Tuple tuple(values, tableSchema_);
            bool tableInsert = tableHeap_->InsertTuple(tuple, &rid);
            index_key.SetFromInteger(Value::GetBigInt(values[0]));
            bool treeInser = tree_->Insert(index_key, rid);
            if(indexRootPageId_ != tree_->GetRootPageId()){
                ChangeIndexRootPageId(tree_->GetRootPageId());   
            }
            return tableInsert && treeInser;
        }

        
        bool GetTuple(uint64_t key, Tuple *tuple){
            GenericKey<64> index_key;
            index_key.SetFromInteger(key);
            std::vector<RID> rids;
            bool treeGet = tree_->GetValue(index_key, &rids);
            if(!treeGet) return false;
            bool tableGet = tableHeap_->GetTuple(rids[0], tuple);
            if(!tableGet) return false;
            return true;
        }
       /* bool GetValue(uint64_t key, const std::string &colName, std::string &v){
            GenericKey<64> index_key;
            index_key.SetFromInteger(key);
            std::vector<RID> rids;
            bool treeGet = tree_->GetValue(index_key, &rids);
            if(!treeGet) return false;
            bool valueGet = tableHeap_->GetTuple(rids[0], tuple);



        }*/
       
        Schema *GetTableSchema() const{
            return tableSchema_;

        }
        void Flush(){
            indexBPM_->FlushAllPages();
            tableBPM_->FlushAllPages();
        }
    private:
        
       // std::mutex mtx_;
        bool isClosed_;
       // std::condition_variable cv_;
        DiskManager *tableDiskManager_;
        DiskManager *indexDiskManager_;
        BufferPoolManager *indexBPM_;
        BufferPoolManager *tableBPM_;
        Schema *tableSchema_;
        TableHeap *tableHeap_;
        BPlusTree<GenericKey<64>, RID, GenericComparator<64>> *tree_;
        page_id_t indexRootPageId_;


    };

}
