
import requests
import datetime
import sqlite3
import json

db = '__HOME__/trivia.db'

def request_handler(request):
    
    if request['method'] == 'GET':
        if 'query' in request.get('args'):
            conn = sqlite3.connect(db)
            c = conn.cursor()
            try:
                to_return = c.execute('''SELECT * FROM bot_req ORDER BY time ASC;''').fetchone()
                if request['values']['query'] == 'cereal':
                    time = to_return[2]
                    c.execute('''DELETE FROM bot_req WHERE time = ?;''',(time,))
                conn.commit()
                conn.close()
            except:
                return '-1'
            
            if not to_return:
                return '-1'

            if request['values']['query'] == 'state':
                return str(to_return[0])

            elif request['values']['query'] == 'cereal':
                return str(to_return[1])
            else:
                return 'Invalid GET'
            
        else:
            return 'Invalid GET'
        
    elif request['method'] == 'POST':
        try:
            data = json.loads(request['data'])
        except:
            return 'invalid POST'
        if 'state' in data and 'cereal' in data:
            state = data['state']
            cereal = data['cereal']
            conn = sqlite3.connect(db)
            c = conn.cursor()
            
            try:
                c.execute('''CREATE TABLE bot_req (instr text, cereal int, time timestamp);''')
            except:
                pass           

            c.execute('''INSERT into bot_req VALUES (?,?,?);''', (state,cereal, datetime.datetime.now()))
            
            conn.commit()
            conn.close()
      
            return 'Success!'
        else:
            return 'Invalid POST'
        
