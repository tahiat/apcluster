#include <limits.h>
#include <float.h>
#include <Rcpp.h>

#include <fstream>

#include "apclusterCppHeaders.h"

using namespace Rcpp;

RcppExport SEXP apclusterC(SEXP sR, SEXP maxitsR, SEXP convitsR,
                           SEXP lamR, SEXP detailsR, SEXP runN, SEXP dataset)
{
    NumericMatrix s(sR);
    int maxits = as<int>(maxitsR);
    int convits = as<int>(convitsR);
    double lam = as<double>(lamR);
    bool details = as<bool>(detailsR);
    int N = s.nrow();
    IntegerMatrix e(N, convits);
    IntegerVector E(N);
    IntegerVector I(N);
    IntegerVector se(N);
    NumericMatrix A(N, N);
    NumericMatrix R(N, N);
    NumericVector tmpidx(N);
    NumericVector netsimAll;
    NumericVector dpsimAll;
    NumericVector exprefAll;
    NumericMatrix idxAll;

    //custom variable start 
    NumericMatrix aa(N, N);
    NumericMatrix ee(N, N);

    int run = as<int>(runN);
    int n_dataset = as<int>(dataset);
    std::string datasets[5] = { "parity5", "Titanic", "dbworld-subjects", "shuttle-landing-control", "analcatdata_vehicle" };
    //custom variable end


    if (details)
    {
        netsimAll = NumericVector(maxits);
        dpsimAll  = NumericVector(maxits);
        exprefAll = NumericVector(maxits);
        idxAll    = NumericMatrix(N, maxits);
    }
	// ------------- LOG ------------------- //
	std::string filename("D:\\NJIT\\RESEARCH\\R-determinism\\Datasets_analysis\\"+datasets[n_dataset]+"\\run"+std::to_string(run)+"\\labels.csv");	// one file will contain all iteration data
	std::ofstream file_out;
	file_out.open(filename, std::ofstream::out);
	// ------------- LOG ------------------- //

    std::string eFileName = "D:\\NJIT\\RESEARCH\\R-determinism\\Datasets_analysis\\"+datasets[n_dataset]+"\\run"+std::to_string(run)+"\\E.csv"; // one file will contain all iteration data
    std::ofstream e_writer;
    e_writer.open(eFileName, std::ofstream::out);

    bool dn = false, unconverged = false;

    int i = 0, j, ii, K;
    int r_variation, a_variation, e_variation;
    while (!dn)
    {


	    // ------------- LOG ------------------- //
	     //file_out << i << ",";
	    // ------------- LOG ------------------- //
        // first, compute responsibilities
        
        for (ii = 0; ii < N; ii++)
        {
            double max1 = -DBL_MAX, max2 = -DBL_MAX, avsim;
            int yMax;
            for (j = 0; j < N; j++) // determine second-largest element of AS
            {
		        avsim = A(ii, j) + s(ii, j);

                if (avsim > max1)
                {
                    max2 = max1;
                    max1 = avsim;
                    yMax = j;
                }
                else if (avsim > max2)
                    max2 = avsim;
            }
            
            for (j = 0; j < N; j++) // perform update
            {
                double oldVal = R(ii, j);
                double newVal = (1 - lam) * (s(ii, j) -
                                (j == yMax ? max2 : max1)) + lam * oldVal;
                R(ii, j) = (newVal > DBL_MAX ? DBL_MAX : newVal);
            }
        }

        
        std::string rFileName = "D:\\NJIT\\RESEARCH\\R-determinism\\Datasets_analysis\\"+ datasets[n_dataset]  +"\\run"+std::to_string(run)+"\\R_" + std::to_string(i) + ".csv";
        std::ofstream r_writer;
        r_writer.open(rFileName, std::ofstream::out);
        r_writer << "RData" << std::endl;  // data are stored in a vector
        for(int t=0;t<N*N;t++){
            r_writer << R[t] << std::endl;   
        }
        r_writer.close();

        // secondly, compute availabilities
        
        for (ii = 0; ii < N; ii++)
        {
            NumericVector Rp(N);
            double auxsum = 0;
            
            for (j = 0; j < N; j++)
            {
                if (R(j, ii) < 0 && j != ii)
                    Rp[j] = 0;
                else
                    Rp[j] = R(j, ii);
                
                auxsum += Rp[j];
            }
            for (j = 0; j < N; j++)
            {
                double oldVal = A(j, ii);
                double newVal = auxsum - Rp[j];
                
                if (newVal > 0 && j != ii)
                    newVal = 0;
                
                A(j, ii) = (1 - lam) * newVal + lam * oldVal;
            }
        }
        
        std::string aFileName = "D:\\NJIT\\RESEARCH\\R-determinism\\Datasets_analysis\\"+datasets[n_dataset]+"\\run"+std::to_string(run)+"\\A_" + std::to_string(i) + ".csv";
        std::ofstream a_writer;  // vector data
        a_writer.open(aFileName, std::ofstream::out);
        a_writer << "AData" << std::endl;
        for(int t=0;t<N*N;t++){
            a_writer << A[t] << std::endl;
        }
        a_writer.close();
        
        // determine clusters and check for convergence
        
        unconverged = false;
        
        K = 0;
        
        for (ii = 0; ii < N; ii++)
        {
            int ex = (A(ii, ii) + R(ii, ii) > 0 ? 1 : 0);
            se[ii] = se[ii] - e(ii, i % convits) + ex;
            if (se[ii] > 0 && se[ii] < convits)
                unconverged = true;
            E[ii] = ex;
            e(ii, i % convits) = ex;
            K += ex;    // ex>0 is potential exemplars. same valued ex are one cluster. 
        }
        
        e_writer << "Iteration " << i ;
        for(int t=0;t<N;t++){
            e_writer << "," << E[t];
        }
        e_writer << std::endl;

        if (i >= (convits - 1) || i >= (maxits - 1))
            dn = ((!unconverged && K > 0) || (i >= (maxits - 1)));
        
        if (K == 0)
        {
            if (details)
            {
                netsimAll[i] = R_NaN;
                dpsimAll[i]  = R_NaN;
                exprefAll[i] = R_NaN;
                
                for (ii = 0; ii < N; ii++)
                    idxAll(ii, i) = R_NaN;
            }
        }
        else
        {
            int cluster = 0;
            
            for (ii = 0; ii < N; ii++)
            {
                if (E[ii])
                {
                    I[cluster] = ii;
                    cluster++;
                }
            }
            //std::cout << "Iter: " << i << std::endl;
            for (ii = 0; ii < N; ii++)
            {
                if (E[ii])
                    tmpidx[ii] = (double)ii;
                else
                {
                    double maxSim = s(ii, I[0]);
                    tmpidx[ii] = (double)I[0];
                    
                    for (j = 1; j < K; j++)
                    {
                        if (s(ii, I[j]) > maxSim)
                        {
                            maxSim = s(ii, I[j]);
                            tmpidx[ii] = (double)I[j];
                        }
                    }
                }

	            // ------------- LOG ------------------- //
		         // file_out << tmpidx[ii] << ",";
	            // ------------- LOG ------------------- //	
		    
            }
	        // ------------- LOG ------------------- //
            // file_out << std::endl;
 	        // ------------- LOG ------------------- //       
	        if (details)
            {
                double sumPref = 0;
                
                for (j = 0; j < K; j++)
                    sumPref += s(I[j], I[j]);
                
                double sumSim = 0;
                
                for (ii = 0; ii < N; ii++)
                {
                    if (!E[ii])
                        sumSim += s(ii, (int)tmpidx[ii]);
                }
                
                netsimAll[i] = sumSim + sumPref;
                dpsimAll[i]  = sumSim;
                exprefAll[i] = sumPref;
                
                NumericMatrix::Column idxLocal = idxAll(_, i);
                idxLocal = tmpidx;
            }
        }
        // dumping cluster assingment of each sampledata to its exemplars.

        file_out << "Iteration " << i;    
        for (int p=0;p<N;p++){
            file_out <<  "," << tmpidx[p];
        }
        file_out << std::endl;
        i++;
    }
    file_out.close();
    e_writer.close();
    List ret;
    ret["I"]      = I;
    ret["K"]      = K;
    ret["it"]     = IntegerVector::create(i - 1);
    ret["unconv"] = LogicalVector::create(unconverged);

	
	//std::cout << "apclusterC.cpp unconv" << LogicalVector::create(unconverged) << std::endl;

	//std::cout << "apclusterC K: " << K << std::endl;
	
    if (details)
    {
        ret["netsimAll"]  = netsimAll;
        ret["dpsimAll"]   = dpsimAll;
        ret["exprefAll"]  = exprefAll;
        ret["idxAll"]     = idxAll;
	//std::cout << "apclusterC.cpp : " << sizeof( idxAll ) << std::endl; 
	//std::cout << "apclusterC.cpp Huehue2 : " << sizeof( idxAll[0] ) << std::endl; 
    }

    return(ret);
}