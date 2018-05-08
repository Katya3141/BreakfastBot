import requests
import sqlite3
import json

db = '__HOME__/dispenser.db'

def request_handler(request):
    
    if request['method'] == 'GET':
        
        conn = sqlite3.connect(db)
        c = conn.cursor()
        try:
            to_return = c.execute('''SELECT * FROM dispenser_state;''').fetchone()
            conn.commit()
            conn.close()
            return str(to_return[0])
        except:
            return '0'
        
    elif request['method'] == 'POST':
        try:
            data = json.loads(request['data'])
            state = data['state']
        except:
            return 'invalid POST ' + str(json.loads(request))
        
        conn = sqlite3.connect(db)
        c = conn.cursor()
        
        try:
            c.execute('''CREATE TABLE dispenser_state (state int);''')
        except:
            pass
        

        c.execute('''DELETE FROM dispenser_state;''')
        c.execute('''INSERT into dispenser_state VALUES (?);''', (state))
        
        conn.commit()
        conn.close()
  
        return 'Success!'
        
        
