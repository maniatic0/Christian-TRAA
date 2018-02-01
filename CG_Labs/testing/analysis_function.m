function [code] = analysis_function(name)
%analysis_function Perform analysis to test 'name'

if exist(name, 'dir') ~= 7 
    disp(strcat('The test folder was not found: ', ...
        name));
    code = -1;
    return; 
end


temporal_filename = fullfile(name, strcat(name, '_temporal.png'));
ground_truth_filename = fullfile(name, strcat(name, '_ground_truth.png'));
no_aa_filename = fullfile(name, strcat(name, '_no_aa.png'));

if exist(temporal_filename, 'file') ~= 2 
    disp(strcat('The temporal test file was not found: ', ...
        temporal_filename));
    code = -1;
    return; 
end

if exist(ground_truth_filename, 'file') ~= 2 
    disp(strcat('The ground truth test file was not found: ', ...
        ground_truth_filename));
    code = -1;
    return;
end

if exist(no_aa_filename, 'file') ~= 2 
    disp(strcat('The no AA test file was not found: ', ...
        ground_truth_filename));
    code = -1;
    return;
end

temporal = imread(temporal_filename);
ground_truth = imread(ground_truth_filename);
no_aa = imread(no_aa_filename);

% Test againts temporal result
[ mse, peaksnr, snr, ssimval, ssimmap, ...
    niqeI, niqeRef, brisqueI, brisqueRef ] ...
    = Test_Files(temporal, ground_truth);

% Test againts no AA result
[ mse_no_aa, peaksnr_no_aa, snr_no_aa, ssimval_no_aa, ssimmap_no_aa, ...
    niqeI_no_aa, ~ , brisqueI_no_aa, ~ ] ...
    = Test_Files(no_aa, ground_truth);

% Save text to diary
diary(fullfile(name, strcat(name, '_results.txt')));

% MSE, Close to zero means it's good
fprintf('\nMSE, Close to zero means it''s good');
fprintf('\n The MSE value of Temporal is %0.4f', mse);
fprintf('\n The MSE value of No AA is %0.4f \n', mse_no_aa);

% PSNR, Close to zero means it's good
fprintf('\nPSNR, Close to zero means it''s good');
fprintf('\n The Peak-SNR value of Temporal is %0.4f', peaksnr);
fprintf('\n The SNR value of Temporal is %0.4f', snr);
fprintf('\n The Peak-SNR value of No AA is %0.4f', peaksnr_no_aa);
fprintf('\n The SNR value of No AA is %0.4f \n', snr_no_aa);

% SSIM, Close to one means it's good
fprintf('\nSSIM, Close to one means it''s good');
fprintf('\n The SSIM value of Temporal is %0.4f',ssimval);
fprintf('\n The SSIM value of No AA is %0.4f \n',ssimval_no_aa);

figure('Name','SSIM Index Map of Temporal'), imshow(ssimmap);
title(sprintf('The SSIM Index Map of Temporal- Mean ssim Value is %0.4f', ...
    ssimval));
savefig(fullfile(name, strcat(name, '_ssim_map_temporal.fig')));
figure('Name','SSIM Index Map of No AA'), imshow(ssimmap_no_aa);
title(sprintf('The SSIM Index Map of No AA - Mean ssim Value is %0.4f', ...
    ssimval_no_aa));
savefig(fullfile(name, strcat(name, '_ssim_map_no_aa.fig')));

% NIQUE, Lower values of score reflect better perceptual quality of image
% with respect to the input MATLAB model
fprintf('\nNIQUE, Lower values of score means it''s good');
fprintf('\n The NIQE score of Temporal is %0.4f', niqeI);
fprintf('\n The NIQE score of Reference is %0.4f', niqeRef);
fprintf('\n The NIQE score of No AA is %0.4f', niqeI_no_aa);
fprintf('\n The NIQE score delta between Temporal and Reference is %+0.4f', ... 
    niqeI - niqeRef);
fprintf('\n The NIQE score delta between Temporal and No AA is %+0.4f \n', ... 
    niqeI - niqeI_no_aa);

% BRISQUE, Lower values of score reflect better perceptual quality of image
% with respect to the input MATLAB model
fprintf('\nBRISQUE, Lower values of score means it''s good');
fprintf('\n The BRISQUE score of Temporal is %0.4f', brisqueI);
fprintf('\n The BRISQUE score of Reference is %0.4f', brisqueRef);
fprintf('\n The BRISQUE score of No AA is %0.4f', brisqueI_no_aa);
fprintf('\n The BRISQUE score delta between Temporal and Reference is %+0.4f', ...
    brisqueI - brisqueRef);
fprintf('\n The BRISQUE score delta between Temporal and No AA is %+0.4f \n', ...
    brisqueI - brisqueI_no_aa);

diary off;
code = 0;
end

