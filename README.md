# SystemSoftwareFinalProject
  The sensor monitoring system consists of sensor nodes measuring the room temperature, a
sensor gateway that acquires all sensor data from the sensor nodes, and an SQL database to
store all sensor data processed by the sensor gateway. A sensor node uses a private TCP
connection to transfer the sensor data to the sensor gateway. The SQL database is an SQLite
system. The full system is depicted below.
The sensor gateway may not assume a maximum amount of sensors at start up. In fact, the
number of sensors connecting to the sensor gateway is not constant and may change over
time.
  Working with real embedded sensor nodes is not an option for this assignment. Therefore,
sensor nodes will be simulated in software
#SENSOR GATEWAY
  The sensor gateway consists of a main process and a log process. The log process is
started (with fork) as a child process of the main process.
The main process runs three threads: the connection, the data, and the storage
manager thread. A shared data structure is used for communication
between all threads. Notice that read/write/update-access to the shared data needs to
be thread-safe (changes in sbuffer.c are required to make it thread-safe)!
  The connection manager listens on a TCP socket for incoming connection requests
from new sensor nodes. The port number of this TCP connection is given as a
command line argument at start-up of the main process, e.g. ./server 1234
  The connection manager captures incoming packets of sensor nodes as defined in 
Next, the connection manager writes the data to the shared data structure.
  The data manager thread implements the sensor gateway intelligence as defined in
In short, it reads sensor measurements from shared data, calculates a running
average on the temperature and uses that result to decide on ‘too hot/cold’. It doesn’t
write the running average values to the shared data – it only uses them for internal
decision taking.
  The storage manager thread reads sensor measurements from the shared data
structure and inserts them in the SQL database. If the connection to the
SQL database fails, the storage manager will wait 5 sec before trying again. The
sensor measurements will stay in shared data until the connection to the database is
working again. If the connection did not succeed after 3 attempts, the gateway will
close.
  The log process receives log-events from the main process using a FIFO called
“logFifo”. If this FIFO doesn’t exists at startup of the main or log process, then it
will be created by one of the processes. All threads of the main process can generate
log-events and write these log-events to the FIFO. This means that the FIFO is
shared by multiple threads and, hence, access to the FIFO must be thread-safe.
  A log-event contains an ASCII info message describing the type of event. For each
log-event received, the log process writes an ASCII message of the format
<sequence number> <timestamp> <log-event info message> to a new line on a log
file called “gateway.log”.
