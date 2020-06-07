#include <mysql/mysql.h>
#include <stdio.h>

void query(MYSQL *conn,char *sql)
{
    int ret;
    MYSQL_RES *res;
    MYSQL_FIELD *field;
    MYSQL_ROW res_row;
    int row, col;
    int i,j;
    

    ret = mysql_query(conn,sql);
    if(ret){
        printf("error\n");
        mysql_close(conn);
    }
    else
    {
        res = mysql_store_result(conn);
        if(res){
            col = mysql_num_fields(res);
            row = mysql_num_rows(res);
            printf("查询到 %d 行\n",row);
            for(i = 0; field = mysql_fetch_field(res); i++)
                printf("%10s ",field->name);
            printf("\n");
            for(i=1;i<row+1;i++){
                res_row = mysql_fetch_row(res);
                for(j=0;j<col;j++)
                    printf("%10s ",res_row[j]);
                printf("\n");
            }
        }
    }
    
}

int main()
{
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;

    char *server = "localhost";
    char *user = "root";
    char *password = "123456";
    char *database = "db_chatroom";

    conn = mysql_init(NULL);

    if(!mysql_real_connect(conn,server,user,password,database,0,NULL,0)){
        fprintf(stderr,"%s\n",mysql_error(conn));
        exit(1);
    }

    // if(mysql_query(conn,"select * from user")){
    //     fprintf(stderr,"%s\n",mysql_error(conn));
    //     exit(1);
    // }

    // res = mysql_use_result(conn);
    // printf("---\n");
    // while((row = mysql_fetch_row(res)) != NULL)
    //     printf("%s %s %s\n",row[0],row[1],row[3]);

    // mysql_free_result(res);
    // mysql_close(conn);

    query(conn,"select * from user");
    return 0;
}