'''
OBJ: {LINKLIST}
VALUE: STR | OBJ
LINK: STR:VALUE
LINKLIST: empty | LINK(,LINK)*
STR: "some text"
'''


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
    s = remove_spaces(s)
    s = eat_obj(s)
    assert s == ''

def eat_obj(s):
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
    s = eat_str(s)
    assert(s[0] == ':')
    s = eat_value(s[1:])
    return s

def eat_str(s):
    assert s and s[0] == '"'
    i = 1
    while i < len(s) and s[i] != '"':
        i += 1
    assert i < len(s)
    return s[i+1:]

def eat_value(s):
    assert s
    if s[0] == '"':
        return eat_str(s)
    return eat_obj(s)

if __name__ == '__main__':
    good = '''{
    "name": "John Doe",
    "age": "30",
    "email": "johndoe@example.com",
    "isEmployed": "true",
    "address": {
        "street": "123 Main St",
        "city": "Anytown",
        "state": "CA",
        "zip": "12345"
    },
    "profile": {
        "username": "johndoe123",
        "bio": "Software developer with a passion for open-source projects.",
        "website": "https://johndoe.dev"
    },
    "lastLogin": "2024-06-14T12:34:56Z"
}'''
    verify(good)

    import traceback
    invalid_jsons = [
    '{"name": "Alice", "age": "25", "email": "alice@example.com"',
    "{'name': 'Bob', 'age': '30', 'email': 'bob@example.com'}",
    '{name: "Charlie", age: "35", email: "charlie@example.com"}',
    '{"name" "Daisy", "age" "28", "email" "daisy@example.com"}',
    '{"name": "Ethan", "age": "40", "email": "ethan@example.com",}'
    ]
    for j in invalid_jsons:
        try:
            verify(invalid_jsons)
            print("error")
            exit(1)
        except AssertionError:
            pass
