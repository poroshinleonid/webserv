import os

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'


print(bcolors.OKBLUE + "Configuring webserv" + bcolors.ENDC)

config_path = './frontend/Config.cfg'
with open(config_path, 'r') as config:
    content = config.read()
    content = content.replace('PATHTOWEBSERV', os.getcwd())

open(config_path, 'w').write(content)

print(bcolors.OKBLUE + "Put path to webserv folder in src/Config.cfg" + bcolors.ENDC)

print(bcolors.OKGREEN + "Finished configuring" + bcolors.ENDC)

print(bcolors.OKGREEN + "Your execution command: " + bcolors.ENDC + "make && ./webserv frontend/Config.cfg")
