#pragma once

#include <boost/program_options.hpp>
#include <string>
#include "boost/filesystem.hpp" 
#include <sstream>

namespace po = boost::program_options; 

class ArgumentParser{

public:
    
    ArgumentParser(int argc, char** argv){m_argc=argc; m_argv=argv; m_brief="";   m_desc.add_options()("help,h","display help");}
    ArgumentParser(){m_argc=0; m_argv=NULL; m_brief="";   m_desc.add_options()("help,h","display help");}
    //ArgumentParser(std::string b){m_brief=b;  m_desc.add_options()("help,h","display help");}
    
    template<typename T>
    void parameter(const char * opt, T& variable, const char * descr, bool required){
        if (required){
            std::stringstream ss;
            ss<<descr<<" (REQUIRED)";
            m_desc.add_options() 
                (opt,po::value<T>(&variable)->required() ,ss.str().c_str()) ;
        }
        else{
            std::stringstream ss;
            ss<<descr<<" (default :"<<variable<<")";
            m_desc.add_options() 
                (opt,po::value<T>(&variable) ,ss.str().c_str()) ;
        }

    }

    void option(const char * opt, bool & variable, const char * descr){
        m_desc.add_options() 
                (opt,po::value<bool>(&variable) ,descr) ;
    }

    bool parse(){
        std::string appName = boost::filesystem::basename(m_argv[0]); 
        po::variables_map vm; 
        try 
            { 
                po::store(po::parse_command_line(m_argc, m_argv, m_desc),  
                          vm); // can throw 
 
                /** --help option 
                 */ 
                if ( vm.count("help")  ) 
                    { 
                        std::cout << m_brief << std::endl <<m_desc<<std::endl;;
                        //rad::OptionPrinter::printStandardAppDesc(appName, 
                        //                                       std::cout, 
                        //                                       m_desc); 
                        exit(0);
                    } 
 
                po::notify(vm); // throws on error, so do after help in case 
                // there are any problems 
            } 
        catch(boost::program_options::required_option& e) 
            { 
                //rad::OptionPrinter::formatRequiredOptionError(e); 
                std::cerr << "ERROR: " << e.what() << std::endl ;
                std::cout<<m_desc<<std::endl; 
                //rad::OptionPrinter::printStandardAppDesc(appName, 
                //                                         std::cout, 
                //                                        m_desc                                               ); 
                exit(0);
            } 
        catch(po::error& e) 
            { 
                std::cerr << "ERROR: " << e.what() << std::endl << std::endl; 
                std::cout << m_desc << std::endl; 
                exit(0);
            } 
    }
    void help(){}

private:
    po::options_description m_desc; 
    std::string m_brief;
    int m_argc;
    char ** m_argv;

};