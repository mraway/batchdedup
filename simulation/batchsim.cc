//Backup Scheduling Simulator
//To add a new scheduler:
//  add a class that inherits from BackupScheduler (see BackupScheduler.h/.cpp)
//  implement schedule_round and getName
//    getName returns a name to print to the user during the run
//    schedule_round returns true if there were vms to schedule, and false if there was nothing to schedule
//    schedule_round fills up the round_schedule vector with machines/vms
//    machines is a variable in the class (of type vector<vector<double> >) containing the remaining vms to schedule and which machine they are on
//    there must be one vector per machine in round_schedule, even if there was nothing scheduled on that machine (the vector count determines p)
//      unless schedule_round returns false, in that case round_schedule is ignored
//    there are some helper functions for CoW and time cost which are defined as extern in BackupScheduler.h
//  add a line to usage() describing the scheduler (under Schedule Types)
//  add 3 lines near the top of main() to parse the command line argument
//    (just copy the nullschedule parsing and change the name, class name)
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <sstream>
#include <string>
//#include "BackupScheduler.h"
#include "../src/RoundScheduler.h"
#include <cmath> //exp
#include <cstdlib> //strtod
#include <cstring> //strcmp

using namespace std;
//double schedule_round(vector<vector<double> > &machine_loads, vector<vector<double> > &round_schedule);
//string format_time(double seconds);
double total_size(const vector<vector<double> > &machine_loads);
double total_size(const vector<map<int, double> > &machine_loads);
//double model_cow_time(const vector<vector<double> > &machine_loads, bool verbose);
//double model_cow_time(double max_size, double total_size, int p, bool verbose);
//double model_time(const vector<vector<double> > &machine_loads, bool verbose);
//double model_time(double max_size, double total_size, int p, bool verbose);
//double model_vm_time(double vm_load, bool verbose);
//double model_vm_cow_time(double vm_load, bool verbose);
//double model_cow(double size, double block_dirty_ratio, double backup_time);
//double model_unneccessary_cow(double size, double block_dirty_ratio, double backup_time);
//double model_round_cow(const vector<vector<double> > &machine_loads);
//double measure_load(const vector<vector<double> > &machine_loads, double &max_size, int &max_mid);
//
////#define DEFAULT_TIME_LIMIT (60*60*3)
//#define DEFAULT_TIME_LIMIT (60*60*3)
////#define DEFAULT_DIRTY_RATIO 0.10
//#define DEFAULT_SEGMENT_DIRTY_RATIO 0.2
//#define DEFAULT_FP_RATIO 0.5
//#define DEFAULT_DUPNEW_RATIO 0.01
//#define DEFAULT_NETWORK_LATENCY 0.0005
//#define DEFAULT_READ_BANDWIDTH 50.0
//#define DEFAULT_DISK_WRITE_BANDWIDTH 50.0
////#define DEFAULT_BACKEND_WRITE_BANDWIDTH 90000000.0
//#define DETECTION_REQUEST_SIZE 40.0
//#define DUPLICATE_SUMMARY_SIZE 48.0
////4KB chunks
//#define CHUNK_SIZE 4096.0
////2MB segments
//#define SEGMENT_SIZE (512.0*CHUNK_SIZE)
////we define sizes in GB
//#define SIZE_UNIT (1<<30)
////memory usage defined in MB
//#define MEMORY_UNIT (1<<20)
////bandwidth defined in MB/s
//#define BANDWIDTH_UNIT (1<<20)
//#define DEFAULT_LOOKUP_TIME 0.000001
////manually computed from x=10,fp=0.5,dirty=0.2,chunk=4096,s=40GB,v=25
//#define DEFAULT_MACHINE_INDEX_SIZE (262144000)
////25MB of net memory leaves 10MB for index parititions, r/w buffers, etc
//#define DEFAULT_NETWORK_MEMORY 25.0
////default write rate defines how many segments are written per second during snapshotting
////20.5 is from 24hr_1GBWrt_all in the EMC paper (the avg writes/second)
////issues with 20.5:
////  probably some writes during a second fall in the same segment, so should be lower (but this is an upper bound)
////  is an average over 24.4 hours; during the backup period (late night) value should be lower
////  number is for general data drives, where we really want vm drives (may increase/decrease value)
////#define DEFAULT_WRITE_RATE 20.5
////I am going with 10 for now just because I think 20.5 is too high
//#define DEFAULT_WRITE_RATE 10.0
//
////cow constant goes into the exponent of the cow calcluation
////this constant still needs to be determined, as it is important to CoW cost
//#define COW_CONSTANT 0.5
//
////These globals define the actual parameters used in modeling
////double dirty_ratio = DEFAULT_DIRTY_RATIO; //block dirty ratio not used
//double time_limit = DEFAULT_TIME_LIMIT;
//double segment_dirty_ratio = DEFAULT_SEGMENT_DIRTY_RATIO;
//double fp_ratio = DEFAULT_FP_RATIO;
//double dupnew_ratio = DEFAULT_DUPNEW_RATIO; //dup-new ratio, as a fration of r
//double net_latency = DEFAULT_NETWORK_LATENCY;
//double read_bandwidth = DEFAULT_READ_BANDWIDTH;
//double disk_write_bandwidth = DEFAULT_DISK_WRITE_BANDWIDTH;
////double backend_write_bandwidth = DEFAULT_BACKEND_WRITE_BANDWIDTH;
//double lookup_time = DEFAULT_LOOKUP_TIME;
//double network_memory = DEFAULT_NETWORK_MEMORY;
//double write_rate = DEFAULT_WRITE_RATE;
//double n = DEFAULT_MACHINE_INDEX_SIZE;
//double u = DETECTION_REQUEST_SIZE;
//double e = DUPLICATE_SUMMARY_SIZE;
//double c = CHUNK_SIZE;

void usage(const char* progname) {
    cout << "Usage: " << progname << " <parameters> <schedule_types>" << endl <<
        "Where <parameters> can be:" << endl <<
        "  --machinefile <path> - file containing list of files with machine loads" << endl <<
        "                       - each machine load should be a size in GB" << endl << 
        "  --segdirty <ratio> - default=" << DEFAULT_SEGMENT_DIRTY_RATIO << endl <<
        "  --fpratio <ratio> - fingerprint dedup success rate; default=" << DEFAULT_FP_RATIO << endl <<
        "  --dupnewratio <ratio> - default=" << DEFAULT_DUPNEW_RATIO << endl <<
        "  --netlatency <seconds> - default=" << DEFAULT_NETWORK_LATENCY << endl <<
        "  --readbandwidth <MB/s> - default=" << DEFAULT_READ_BANDWIDTH << endl <<
        "  --diskwritebandwidth <MB/s> - default=" << DEFAULT_DISK_WRITE_BANDWIDTH << endl <<
        "  --timelimit <seconds> - default=" << DEFAULT_TIME_LIMIT << endl <<
        //"  --backwritebandwidth <MB/s> - default=" << DEFAULT_BACKEND_WRITE_BANDWIDTH << endl <<
        "  --indexsize <machine_index_entries> - default=" << DEFAULT_MACHINE_INDEX_SIZE << endl <<
        "Schedule Types:" << endl <<
        "  --nullschedule - just schedule all the jobs at once" << endl <<
        //"  --oneschedule - schedule one vm in each round" << endl <<
        //"  --oneeachschedule - schedule one vm on each machine in each round" << endl <<
        //"  --cowschedule - basic cow based scheduler" << endl <<
        //"  --dbpschedule - scheduler based on Dual Bin Packing (involves uCoW)" << endl <<
        "  --dbpschedule2 - improved scheduler based on Dual Bin Packing (uses time only)" << endl << 
        //"  --bpschedule - machine by machine scheduler which uses basic bin packing" << endl << 
        //"  --dbpn1schedule - Naive Dual Bin Packing scheduler" << endl <<
        //"  --dbpn2schedule - Naive Dual Bin Packing scheduler 2" << 
        "  --bplschedule - bin packing scheduler using binary search" << endl << 
        endl;
}

vector<map<int,double> > remap_schedule(vector<map<int,double> > loads, vector<vector<int> > schedule) {
    vector<map<int,double> > outsch;
    for(int mid = 0; mid < schedule.size(); mid++) {
        map<int,double> outmap;
        for(vector<int>::const_iterator vm = schedule[mid].begin(); vm != schedule[mid].end(); ++vm) {
            outmap[*vm] = loads[mid][*vm];
        }
        outsch.push_back(outmap);
    }
    return outsch;
}

int main (int argc, char *argv[]) {
    int argi = 1;
    const char* machinefile = NULL;
    char *endptr;
    vector<RoundScheduler*> schedulers;
    double time_limit = DEFAULT_TIME_LIMIT;
    double alpha = 0.5;
    while(argi < argc && argv[argi][0] == '-') {
        if (!strcmp(argv[argi],"--")) {
            argi++; break;
        }
        //here is the arg parsing for different schedulers
        else if (!strcmp(argv[argi],"--nullschedule")) {
            argi++;
            schedulers.push_back(new NullScheduler());
        //} else if (!strcmp(argv[argi],"--cowschedule")) {
        //    argi++;
        //    schedulers.push_back(new CowScheduler());
        //} else if (!strcmp(argv[argi],"--oneeachschedule")) {
        //    argi++;
        //    schedulers.push_back(new OneEachScheduler());
        //} else if (!strcmp(argv[argi],"--oneschedule")) {
        //    argi++;
        //    schedulers.push_back(new OneScheduler());
        //} else if (!strcmp(argv[argi],"--dbpschedule")) {
        //    argi++;
        //    schedulers.push_back(new DBPScheduler());
        } else if (!strcmp(argv[argi],"--dbpschedule2")) {
            argi++;
            schedulers.push_back(new DBPScheduler2());
        //} else if (!strcmp(argv[argi],"--dbpn1schedule")) {
        //    argi++;
        //    schedulers.push_back(new DBPN1Scheduler());
        //} else if (!strcmp(argv[argi],"--dbpn2schedule")) {
        //    argi++;
        //    schedulers.push_back(new DBPN2Scheduler());
        } else if (!strcmp(argv[argi],"--bplschedule")) {
            argi++;
            schedulers.push_back(new BPLScheduler());
        } else if (!strcmp(argv[argi],"--bplschedule2")) {
            argi++;
            schedulers.push_back(new BPLScheduler2(alpha));
        //} else if (!strcmp(argv[argi],"--bpgschedule")) {
        //    argi++;
        //    schedulers.push_back(new BinPackGlobalScheduler());
        }
        //Below here is the arg parsing for modeling parameters
        else if (!strcmp(argv[argi],"--machinefile") || !strcmp(argv[argi],"-m")) {
            machinefile=argv[++argi];
            argi++;
        } else if (!strcmp(argv[argi],"--segdirty")) {
            segment_dirty_ratio = strtod(argv[++argi],&endptr);
            if (endptr == argv[argi]) {
                cout << "invalid dirty ratio or none given";
                usage(argv[0]);
                return -1;
            }
            argi++;
        } else if (!strcmp(argv[argi],"--fpratio")) {
            fp_ratio = strtod(argv[++argi],&endptr);
            if (endptr == argv[argi]) {
                cout << "invalid fp ratio or none given";
                usage(argv[0]);
                return -1;
            }
            argi++;
        } else if (!strcmp(argv[argi],"--schalpha")) {
            alpha = strtod(argv[++argi],&endptr);
            if (endptr == argv[argi]) {
                cout << "invalid scheduler alpha or none given";
                usage(argv[0]);
                return -1;
            }
            argi++;
        } else if (!strcmp(argv[argi],"--dupnewratio")) {
            dupnew_ratio = strtod(argv[++argi],&endptr);
            if (endptr == argv[argi]) {
                cout << "invalid dupnew ratio or none given";
                usage(argv[0]);
                return -1;
            }
            argi++;
        } else if (!strcmp(argv[argi],"--netlatency")) {
            net_latency = strtod(argv[++argi],&endptr);
            if (endptr == argv[argi]) {
                cout << "invalid network latency or none given";
                usage(argv[0]);
                return -1;
            }
            argi++;
        } else if (!strcmp(argv[argi],"--readbandwidth")) {
            read_bandwidth = strtod(argv[++argi],&endptr);
            if (endptr == argv[argi]) {
                cout << "invalid disk bandwidth or none given";
                usage(argv[0]);
                return -1;
            }
            read_bandwidth *= BANDWIDTH_UNIT;
            argi++;
        } else if (!strcmp(argv[argi],"--diskwritebandwidth")) {
            disk_write_bandwidth = strtod(argv[++argi],&endptr);
            if (endptr == argv[argi]) {
                cout << "invalid disk bandwidth or none given";
                usage(argv[0]);
                return -1;
            }
            disk_write_bandwidth *= BANDWIDTH_UNIT;
            argi++;
        //} else if (!strcmp(argv[argi],"--backwritebandwidth")) {
        //    if (endptr == argv[argi]) {
        //        cout << "invalid disk bandwidth or none given";
        //        usage(argv[0]);
        //        return -1;
        //    }
        //    backend_write_bandwidth = strtod(argv[++argi],&endptr);
        //    backend_write_bandwidth *= BANDWIDTH_UNIT;
        //    argi++;
        } else if (!strcmp(argv[argi],"--timelimit")) {
            time_limit = strtod(argv[++argi],&endptr);
            if (endptr == argv[argi]) {
                cout << "invalid time limit or none given";
                usage(argv[0]);
                return -1;
            }
            argi++;
        } else if (!strcmp(argv[argi],"--netmemory")) {
            network_memory = strtod(argv[++argi],&endptr);
            if (endptr == argv[argi]) {
                cout << "invalid memory amount or none given";
                usage(argv[0]);
                return -1;
            }
            //convert memory quantity to bytes
            network_memory *= MEMORY_UNIT;
            argi++;
        } else if (!strcmp(argv[argi],"--indexsize")) {
            n = strtod(argv[++argi],&endptr);
            if (endptr == argv[argi]) {
                cout << "invalid machine index size or none given";
                usage(argv[0]);
                return -1;
            }
            argi++;
        } else {
            cout << "Unknown argument: " << argv[argi] << endl;
            usage(argv[0]);
            return -1;
        }
    }

    if (machinefile == NULL) {
        cout << "must specify machine file" << endl;
        usage(argv[0]);
        return -1;
    }

    cout << print_settings(time_limit);
    
    

    vector<map<int,double> > machinelist;

    ifstream machine_list_stream;
    ifstream machine_vm_stream;
    string line;
    machine_list_stream.open(machinefile);
    if (!machine_list_stream.is_open()) {
        cout << "could not open machine file (" << machinefile << ")" << endl;
        return -1;
    }

    //read in the machine load files (to machinelist)
    int mid = 0;
    while(getline(machine_list_stream,line)) {
        if (line.length() < 1 || line[0] == '\n') {
            continue;
        }
        machine_vm_stream.open(line.c_str());
        if (!machine_vm_stream.is_open()) {
            cout << "could not open machine vm list (" << line << ")" << endl;
            return -1;
        }
        map<int,double> vmlist;
        double size;
        while(machine_vm_stream >> size) {
            vmlist[mid++] = size;
        }
        machine_vm_stream.close();
        machinelist.push_back(vmlist);
    }
    machine_list_stream.close();

    //for(int i = 0; i < machinelist.size(); i++) {
    //    for(int j = 0; j < machinelist[i].size(); j++) {
    //        cout << machinelist[i][j] << endl;
    //    }
    //}

    //get total size/count of the vms
    int total_vms = 0;
    double total_vm_size = 0;
    for(vector<map<int,double> >::const_iterator machine = machinelist.begin(); machine != machinelist.end(); ++machine) {
        for(map<int,double>::const_iterator vm = (*machine).begin(); vm != (*machine).end(); ++vm) {
            total_vms++;
            total_vm_size += vm->second;
        }
    }

    //here we run all the schedulers, and use the model to gather some statistics
    for(std::vector<RoundScheduler*>::iterator scheduler = schedulers.begin(); scheduler != schedulers.end(); ++scheduler) {
        int round = 1;
        double total_time = 0;
        //double total_cow = 0;
        double avg_backup = 0;
        vector<vector<int> > schedule;
        cout << "Total data to schedule: " << total_size(machinelist) << "GB" << endl;
        (*scheduler)->setMachineList(machinelist);
        (*scheduler)->setTimeLimit(time_limit);
        cout << "Scheduler: " << (*scheduler)->getName() << endl;
        while((*scheduler)->schedule_round(schedule)) {
            double round_time = model_time(remap_schedule(machinelist,schedule), true);
            //double round_cow = model_round_cow(schedule);
            int round_vms = 0;
            double round_size = 0;
            //cout << "Round loads:" << endl;
            for(int mid = 0; mid < schedule.size(); mid++) {
                round_vms += schedule[mid].size();
                //cout << "  >";
                for(vector<int>::const_iterator vm = schedule[mid].begin(); vm != schedule[mid].end(); ++vm) {
                    round_size += machinelist[mid][*vm];
                    //cout << " " << *vm;
                }
                //cout << endl;
            }
            cout << "  Round " << round << ": Data Size=" << round_size << "GB; VMs=" << round_vms << "; Round Time=" << format_time(round_time) << 
                //"; Round CoW: " << round_cow << "GB" <<
                endl;
            total_time += round_time;
            //total_cow += round_cow;
            avg_backup += round_time * round_vms;
            schedule.clear();
            round++;
        }
        cout << "  Total Data: " << total_vm_size << "GB; Total VMs: " << total_vms << "; Total Time: " << format_time(total_time) << 
            "; Total Avg VM Time: " << format_time(avg_backup/total_vms) << 
            //"; CoW Cost: " << total_cow << "GB" <<
            endl;
    }
    //cleanup
    for(std::vector<RoundScheduler*>::iterator it = schedulers.begin(); it != schedulers.end(); ++it) {
        delete *it;
    }
    schedulers.clear();
}

////formats time into raw seconds and hours/minutes/seconds
//string format_time(double seconds) {
//    stringstream ss;
//    ss << seconds << " seconds (";
//    int hours = 0;
//    int minutes = 0;
//    if (seconds > 3600) {
//        hours = (int)(seconds / 3600);
//        seconds -= 3600 * hours;
//        ss << hours << "h";
//    }
//    if (seconds > 60) {
//        minutes = (int)(seconds / 60);
//        seconds -= 60 * minutes;
//        ss << minutes << "m";
//    }
//    ss << seconds << "s)";
//    return ss.str();
//}

//gets the total load for a set of machine loads
double total_size(const vector<vector<double> > &machine_loads) {
    double total_size = 0;
    for(vector<vector<double> >::const_iterator machine = machine_loads.begin(); machine != machine_loads.end(); ++machine) {
        double machine_cost = 0;
        for(vector<double>::const_iterator vm = (*machine).begin(); vm != (*machine).end(); ++vm) {
            machine_cost += (*vm);
        }
        total_size += machine_cost;
    }
    return total_size;
}
double total_size(const vector<map<int,double> > &machine_loads) {
    double total_size = 0;
    for(vector<map<int,double> >::const_iterator machine = machine_loads.begin(); machine != machine_loads.end(); ++machine) {
        double machine_cost = 0;
        for(map<int,double>::const_iterator vm = (*machine).begin(); vm != (*machine).end(); ++vm) {
            machine_cost += vm->second;
        }
        total_size += machine_cost;
    }
    return total_size;
}

//double model_time(double max_size, double total_size, int p, bool verbose) {
//    double time_cost = 0;
//    double b_r = read_bandwidth;
//    double b_w = disk_write_bandwidth;
//    //double b_b = backend_write_bandwidth;
//    double m_n = network_memory;
//
//    double r = (total_size * segment_dirty_ratio / c / p) * SIZE_UNIT; //avg num requests per machine
//    double r_max = (max_size * segment_dirty_ratio / c) * SIZE_UNIT; //requests from most heavily loaded machine
//    double r2_max = r_max * (1-fp_ratio); //number of new blocks at most heavily loaded machine; Not a real variable, but used often
//    //cout << "r=" << r << "; r_max=" << r_max << endl;
//    double t;
//
//    //Stage 1a - exchange dirty data
//    t = r_max * c / b_r; //read from disk
//    t += net_latency * (u * r_max / m_n); //transfer dirty data
//    t += u * r / b_w; //save requests to disk
//    if (verbose) {
//        cout << "    Stage 1a: " << format_time(t) << endl;
//    }
//    time_cost += t;
//    //Stage 1b - handle dedup requests
//    t = r * u / b_r; //read requests from disk
//    t += n*e/b_r; //read one machine's worth of index from disk
//    t += r * lookup_time; //perform lookups
//    t += r * e / b_w; //write out results of each lookup to one of 3 files per partition
//    if (verbose) {
//        cout << "    Stage 1b: " << format_time(t) << endl;
//    }
//    time_cost += t;
//    //Stage 2a - exchange new block results
//    t = r * (1-fp_ratio) * e / b_r; //read new results from disk
//    t += net_latency * (e * r2_max / m_n); //exchange new blocks
//    t += r2_max * e / b_w; //save new block results to disk
//    if (verbose) {
//        cout << "    Stage 2a: " << format_time(t) << endl;
//    }
//    time_cost += t;
//    //Stage 2b - save new blocks
//    t = r2_max * e / b_r; //read new results from disk
//    t += r_max * c / b_r; //re-read dirty blocks
//    //t += r2_max * c / b_b; //write new blocks to the backend - also update the lookup results
//    if (verbose) {
//        cout << "    Stage 2b: " << format_time(t) << endl;
//    }
//    time_cost += t;
//    //Stage 3a - exchange new block references
//    t = r2_max * e / b_r; //re-read new block results from disk (now with references)
//    t += net_latency * (e * r2_max / m_n); //exchange new block references back to parition holders
//    t += r * (1-fp_ratio) * e / b_w; //save new block references at partition holder
//    if (verbose) {
//        cout << "    Stage 3a: " << format_time(t) << endl;
//    }
//    time_cost += t;
//    //Stage 3b - update index with new blocks and update dupnew results with references
//    t = r * (1-fp_ratio) * e / b_r; //read new blocks for each partition from disk
//    t += r * dupnew_ratio * e / b_r; //read dupnew results for each partition
//    t += r * dupnew_ratio * lookup_time; //get references to the dupnew blocks
//    t += r * dupnew_ratio * e / b_w; //add updated dup-new results to the list of dup results
//    t += r * (1-fp_ratio) * e / b_w; //add new blocks to local partition index
//    if (verbose) {
//        cout << "    Stage 3b: " << format_time(t) << endl;
//    }
//    time_cost += t;
//    //Stage 4a - exchange dup (including dupnew) references
//    t = r * fp_ratio * e / b_r; //read dup results from disk (including dupnew)
//    t += net_latency * (e * r_max * fp_ratio / m_n); //exchange references to dup blocks
//    t += r_max * fp_ratio * e / b_w; //save dup references to disk
//    if (verbose) {
//        cout << "    Stage 4a: " << format_time(t) << endl;
//    }
//    time_cost += t;
//    //Stage 4b - sort and save recipes back to storage service (we ignore this for now)
//
//    return time_cost;
//
//}
//
//double model_cow_time(double max_size, double total_size, int p, bool verbose) {
//    double time_cost = 0;
//    double b_r = read_bandwidth;
//    double b_w = disk_write_bandwidth;
//    //double b_b = backend_write_bandwidth;
//    double m_n = network_memory;
//
//    double r = (total_size * segment_dirty_ratio / c / p) * SIZE_UNIT; //avg num requests per machine
//    double r_max = (max_size * segment_dirty_ratio / c) * SIZE_UNIT; //requests from most heavily loaded machine
//    double r2_max = r_max * (1-fp_ratio); //number of new blocks at most heavily loaded machine; Not a real variable, but used often
//    //cout << "r=" << r << "; r_max=" << r_max << endl;
//    double t;
//
//    //Stage 1a - exchange dirty data
//    t = r_max * c / b_r; //read from disk
//    t += net_latency * (u * r_max / m_n); //transfer dirty data
//    t += u * r / b_w; //save requests to disk
//    if (verbose) {
//        cout << "    Stage 1a: " << format_time(t) << endl;
//    }
//    time_cost += t;
//    //Stage 1b - handle dedup requests
//    t = r * u / b_r; //read requests from disk
//    t += n*e/b_r; //read one machine's worth of index from disk
//    t += r * lookup_time; //perform lookups
//    t += r * e / b_w; //write out results of each lookup to one of 3 files per partition
//    if (verbose) {
//        cout << "    Stage 1b: " << format_time(t) << endl;
//    }
//    time_cost += t;
//    //Stage 2a - exchange new block results
//    t = r * (1-fp_ratio) * e / b_r; //read new results from disk
//    t += net_latency * (e * r2_max / m_n); //exchange new blocks
//    t += r2_max * e / b_w; //save new block results to disk
//    if (verbose) {
//        cout << "    Stage 2a: " << format_time(t) << endl;
//    }
//    time_cost += t;
//    //Stage 2b - save new blocks
//    t = r2_max * e / b_r; //read new results from disk
//    t += r_max * c / b_r; //re-read dirty blocks
//    //t += r2_max * c / b_b; //write new blocks to the backend - also update the lookup results
//    if (verbose) {
//        cout << "    Stage 2b: " << format_time(t) << endl;
//    }
//    time_cost += t;
//
//    return time_cost;
//}
//
//double measure_load(const vector<vector<double> > &machine_loads, double &max_size, int &max_mid) {
//    max_size = 0;
//    max_mid = 0;
//    double total_size = 0;
//    for(int i = 0; i < machine_loads.size(); i++) {
//        double machine_size = 0;
//        for(vector<double>::const_iterator vm = machine_loads[i].begin(); vm != machine_loads[i].end(); ++vm) {
//            machine_size += (*vm);
//        }
//        if (machine_size > max_size) {
//            max_size = machine_size;
//            max_mid = i;
//        }
//        total_size += machine_size;
//    }
//    return total_size;
//}
//
////uses the time model to compute how long a given round schedule would take, and optionally prints stage times
////only goes through stage 2b (which is when CoW can be released)
//double model_cow_time(const vector<vector<double> > &machine_loads, bool verbose) {
//    double max_size;
//    int mid;
//    double total_size = measure_load(machine_loads, max_size, mid);
//    return model_cow_time(max_size,total_size, machine_loads.size(),verbose);
//}
//
////uses the time model to compute how long a given round schedule would take, and optionally prints stage times
//double model_time(const vector<vector<double> > &machine_loads, bool verbose) {
//    double max_size;
//    int mid;
//    double total_size = measure_load(machine_loads, max_size, mid);
//    return model_time(max_size,total_size, machine_loads.size(),verbose);
//}
//
////uses the time model to compute how long a given round schedule would take, and optionally prints stage times
////allows for modification of the load on one machine (adds load of size <vmsize> to machine <mid>
//double model_time2(const vector<vector<double> > &machine_loads, int mid, int vmsize, bool verbose) {
//    double max_size = 0;
//    double total_size = 0;
//    for(int i = 0; i < machine_loads.size(); i++) {
//        double machine_size = 0;
//        if (i == mid) {
//            machine_size += vmsize;
//        }
//        for(vector<double>::const_iterator vm = machine_loads[i].begin(); vm != machine_loads[i].end(); ++vm) {
//            machine_size += (*vm);
//        }
//        if (machine_size > max_size) {
//            max_size = machine_size;
//        }
//        total_size += machine_size;
//    }
//    return model_time(max_size,total_size, machine_loads.size(),verbose);
//}
//
////gets the minimum backup time for a given vm (backing up only that vm)
//double model_vm_time(double vm_load, bool verbose) {
//    vector<vector<double> > machines;
//    vector<double> machine;
//    machine.push_back(vm_load);
//    machines.push_back(machine);
//    return model_time(machines,verbose);
//}
//
////gets the minimum backup CoW time for a given vm (backing up only that vm)
//double model_vm_cow_time(double vm_load, bool verbose) {
//    vector<vector<double> > machines;
//    vector<double> machine;
//    machine.push_back(vm_load);
//    machines.push_back(machine);
//    return model_cow_time(machines,verbose);
//}
//
////uses the CoW model to estimate how many SIZE_UNITs of space CoW would require during a backup round
//double model_cow(double size, double block_dirty_ratio, double backup_time) {
//    double segments = size*SIZE_UNIT/SEGMENT_SIZE;
//    double cow = segments*segment_dirty_ratio*(1-exp(-(write_rate*backup_time*COW_CONSTANT) / segments ))*SEGMENT_SIZE/SIZE_UNIT;
//
//    //cout << "size: " << size << "; seg:" << segments << "; dirty: " << segment_dirty_ratio << "; time: " << backup_time << "; cow: " << cow << endl;
//    return cow;
//}
//
//double model_unneccessary_cow(double size, double block_dirty_ratio, double backup_time) {
//    return model_cow(size, block_dirty_ratio, backup_time)-model_cow(size, block_dirty_ratio, model_vm_cow_time(size, false));
//}
//
//double model_round_cow(const vector<vector<double> > &machine_loads) {
//    //double biggest_machine = 0;
//    double cow = 0;
//    double round_cow_time = model_cow_time(machine_loads, false);
//    //for(vector<vector<double> >::const_iterator machine = machine_loads.begin(); machine != machine_loads.end(); ++machine) {
//    //    double machine_cost = 0;
//    //    for(vector<double>::const_iterator vm = (*machine).begin(); vm != (*machine).end(); ++vm) {
//    //        machine_cost += (*vm);
//    //    }
//    //    if (machine_cost > biggest_machine) {
//    //        biggest_machine = machine_cost;
//    //    }
//    //}
//
//    for(vector<vector<double> >::const_iterator machine = machine_loads.begin(); machine != machine_loads.end(); ++machine) {
//        //double machine_cost = 0;
//        for(vector<double>::const_iterator vm = (*machine).begin(); vm != (*machine).end(); ++vm) {
//            //machine_cost += (*vm);
//            cow += model_cow(*vm, segment_dirty_ratio, round_cow_time);
//        }
//        //insert CoW cost analysis here
//    }
//
//    return cow;
//}
