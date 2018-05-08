
import requests
import sqlite3
import json

db = '__HOME__/trivia.db'

def request_handler(request):
    
    if request['method'] == 'GET':
        
        conn = sqlite3.connect(db)
        c = conn.cursor()
        try:
            to_return = c.execute('''SELECT * FROM params;''').fetchone()
            conn.commit()
            conn.close()
            return str(to_return[0]) + ' ' + str(to_return[1]) + ' ' + str(to_return[2]) + ' ' + str(to_return[3]) + ' ' + str(to_return[4])
        except:
            return '0'
        
    elif request['method'] == 'POST':
        try:
            data = json.loads(request['data'])
            p = data['p']
            i = data['i']
            d = data['d']
            instr = data['instr']
            max_curve = data['max_curve']
        except:
            return 'invalid POST'
        
        conn = sqlite3.connect(db)
        c = conn.cursor()
        
        try:
            c.execute('''CREATE TABLE params (p float, i float, d float, instr String, max_curve float);''')
        except:
            pass
        

        c.execute('''DELETE FROM params;''')
        c.execute('''INSERT into params VALUES (?,?,?,?,?);''', (p,i,d, instr, max_curve))
        
        conn.commit()
        conn.close()
  
        return 'Success!'
        
        