'''
OBJ: {LINKLIST}
VALUE: STR | OBJ
LINK: STR:VALUE
LINKLIST: empty | LINK(,LINK)*
STR: "some text"
'''

# put those 2 values in class / namespace to avoid globals
value_found = ""
key_to_find = "age"
key_found = ""

def remove_spaces(s):
    i = 0
    res = []
    while i < len(s):
        if (s[i] == '"'):
            res.append('"')
            i += 1
            while i < len(s) and s[i] != '"':
                res.append(s[i])
                i += 1
        if i < len(s) and not s[i].isspace():
            res.append(s[i])
        i += 1
    return ''.join(res)

def verify(s):
    global key_found, value_found

    key_found = ""
    value_found = ""
    s = remove_spaces(s) # move it outside to call it only once
    s = eat_obj(s)
    assert s == ''

def eat_obj(s):
    global key_found, value_found, key_to_find
    assert len(s) >= 2 and s[0] == '{'
    in_quote = False
    open_curly = 1
    i = 1
    while i < len(s):
        if s[i] == '"':
            in_quote = not in_quote
        if not in_quote and s[i] == '{':
            open_curly += 1
        if not in_quote and s[i] == '}':
            open_curly -= 1
            if open_curly == 0:
                if key_found == key_to_find:
                    value_found = s[:i+1]
                verify_linklist(s[1:i])
                return s[i+1:]
        i += 1
    assert False

def verify_linklist(s):
    while s:
        s = eat_link(s)
        if not s:
            return
        assert s[0] == ','
        s = s[1:]

def eat_link(s):
    global key_found

    assert s and s[0] == '"'
    i = s.find('"', 1)
    assert i != -1
    key_found = s[1:i]
    s = s[i+1:]
    assert(s[0] == ':')
    s = eat_value(s[1:])
    return s

def eat_value(s):
    global value_found, key_found, key_to_find

    assert s
    if s[0] == '"':
        i = s.find('"', 1)
        assert i != -1
        if key_found == key_to_find:
            value_found = s[1:i]
            key_found = ""
        return s[i+1:]
    return eat_obj(s)

if __name__ == '__main__':
    good = '''{
    "name": "John Doe",
    "street": "30",
    "email": "johndoe@example.com",
    "isEmployed": "true",
    "age": {
        "city": "Anytown",
        "state": "CA",
        "zip": "12345",
        "a": {"b":"c"}
    },
    "profile": {
        "username": "johndoe123",
        "bio": "Software developer with a passion for open-source projects.",
        "website": "https://johndoe.dev"
    },
    "lastLogin": "2024-06-14T12:34:56Z"
}'''
    verify(good)
    print(value_found)
    # invalid_jsons = [
    # '{"name": "Alice", "age": "25", "email": "alice@example.com"',
    # "{'name': 'Bob', 'age': '30', 'email': 'bob@example.com'}",
    # '{name: "Charlie", age: "35", email: "charlie@example.com"}',
    # '{"name" "Daisy", "age" "28", "email" "daisy@example.com"}',
    # '{"name": "Ethan", "age": "40", "email": "ethan@example.com",}',
    # '{"":"b"}'
    # ]
    # for j in invalid_jsons:
    #     try:
    #         verify(invalid_jsons)
    #         print("should have failed")
    #         exit(1)
    #     except AssertionError:
    #         pass
