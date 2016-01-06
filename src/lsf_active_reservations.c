/*
 * Copyright (c) 2015, University Corporation for Atmospheric Research
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <map>
#include <set>
#include <cstdlib>
#include <iostream>
#include <string>
#include <regex>

extern "C"
{
#include "lsf/lsbatch.h"
#include "lsf/lsf.h"   
}

#define ADVRSV_STATE_UNKNOWN        0
#define ADVRSV_STATE_INACTIVE       1
#define ADVRSV_STATE_PRE_STARTED    2
#define ADVRSV_STATE_PRE_FAILED     3
#define ADVRSV_STATE_ACTIVE         4
#define ADVRSV_STATE_POST_STARTED   5

int init_lsf(char* program_name)
{
    /* initialize LSBLIB and get the configuration environment */
    if (lsb_init(program_name) < 0) {
        lsb_perror(const_cast<char *>("lsb_init"));
        return EXIT_FAILURE;
    } 
    return EXIT_SUCCESS;
}

template<typename map_t> size_t sum_values(const map_t &map)
{
    size_t sum = 0;

    typename map_t::const_iterator end_itr = map.end();
    for(typename map_t::const_iterator itr = map.begin(); itr != end_itr; ++itr)
        sum += itr->second;

    return sum;
}

int main(int argc, char **argv)
{
    using namespace std;
    if(init_lsf(argv[0]))
        return EXIT_FAILURE;

    if(argc != 3)
    {
        cerr << argv[0] << " {host regex} {label}" << endl << 
            "Dumps LSF Advance reservation statistics against host regex" << endl  <<
            "format: type hosts slots reservations" << std::endl;
        return EXIT_FAILURE;
    }

    const string label(argv[2]);
    regex host_regex(argv[1]);

    //Use map to stack slots incase of shared nodes
    map<string, size_t> sys_hosts, usr_hosts;
    size_t sys_rsv = 0, usr_rsv = 0;

    struct rsvInfoEnt * rsvEntries;
    int numEnts;
    
    rsvEntries = lsb_reservationinfo(NULL, &numEnts, 0);
    if(rsvEntries && numEnts > 0)
    {
        for(size_t i = 0; i < numEnts; ++i)
        {
            struct rsvInfoEnt & rsv = rsvEntries[i];

            if(rsv.state == ADVRSV_STATE_ACTIVE)
            {
                ///Match against host regex before considering reservation
                bool found = false;
                for(size_t hi = 0; hi < rsv.numRsvHosts; ++hi)
                {
                    struct hostRsvInfoEnt & host = rsv.rsvHosts[hi];

                    if(regex_search(host.host, host_regex))
                    {
                        found = true;
                        break;
                    }
                }
                if(!found) continue;

                if(rsv.options & RSV_OPTION_SYSTEM)
                    ++sys_rsv;
                else //RSV_OPTION_USER | RSV_OPTION_GROUP
                    ++usr_rsv;

                for(size_t hi = 0; hi < rsv.numRsvHosts; ++hi)
                {
                    struct hostRsvInfoEnt & host = rsv.rsvHosts[hi];

                    if(rsv.options & RSV_OPTION_SYSTEM)
                        sys_hosts[host.host] += host.numSlots;
                    else //RSV_OPTION_USER | RSV_OPTION_GROUP
                        usr_hosts[host.host] += host.numSlots;
                }
            }
        }
    }
    else if(lsberrno)
    {
        lsb_perror(const_cast<char *>("lsb_reservationinfo"));
        return EXIT_FAILURE;
    }

    //std::cout << "type hosts slots reservations" << std::endl <<
    std::cout << 
        label << "system " << sys_hosts.size() << " " << sum_values(sys_hosts) << " " << sys_rsv << endl <<
        label << "user " << usr_hosts.size() << " " << sum_values(usr_hosts) << " " << usr_rsv << endl;

    return EXIT_SUCCESS;
}

