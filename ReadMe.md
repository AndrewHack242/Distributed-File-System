How to use:
add login info to dfs.conf and swap out login info in dfc.conf
login is read at launch of client, will not update until program is restarted

Each server has its own folder, for proper use, run all with ./server [port] (default ports are 10001 - 10004 in dfc.conf)

the client can be run with ./client dfc.conf

The distributed file system uses a very basic XOR encryption scheme based on the user's password.
CHANGING THE PASSWORD IN THE DFC.CONF AND DFS.CONF WILL RENDER ALL PREVIOUS FILES USELESS AS THEY WILL NOT BE DECRYPTED PROPERLY

once you are in, your possible commands are list, put [filename], and get [filename]