function [code] = analysis_function(name)
%analysis_function Perform analysis to test 'name'

if exist(name, 'dir') ~= 7 
    fprintf('The test folder was not found: %s\n', name);
    code = -1;
    return; 
end


results_filename = fullfile(name, strcat(name, '_results.txt'));

if exist(results_filename, 'file') == 2 
    fprintf('Results File found: %s\n', results_filename);
    fprintf('The test %s was Already Performed, Skipping\n', ...
        name);
    code = 1;
    return; 
end

results_extra_filename = fullfile(name, strcat(name, '_results_extra.txt'));


temporal_filename = fullfile(name, strcat(name, '_temporal.png'));
ground_truth_filename = fullfile(name, strcat(name, '_ground_truth.png'));
no_aa_filename = fullfile(name, strcat(name, '_no_aa.png'));

if exist(temporal_filename, 'file') ~= 2 
    fprintf('The temporal test file was not found: %s\n', ...
        temporal_filename);
    code = -1;
    return; 
end

if exist(ground_truth_filename, 'file') ~= 2 
    fprintf('The ground truth test file was not found: %s\n', ...
        ground_truth_filename);
    code = -1;
    return;
end

if exist(no_aa_filename, 'file') ~= 2 
    fprintf('The no AA test file was not found: %s\n', ...
        no_aa_filename);
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

% Main Results
% Save text to diary
diary(results_filename);
fprintf('Timestamp(dd/mm/yyyy): %s\n', datestr(now,'dd/mm/yyyy HH:MM:SS.FFF'))

% MSE, Close to zero means it's good
fprintf('\nMSE and RMSE, Close to zero means it''s good');
fprintf('\n The MSE value of Temporal is %0.6f', sqrt(mse));
fprintf('\n The RMSE value of Temporal is %0.6f', mse);
fprintf('\n The MSE value of No AA is %0.6f \n', mse_no_aa);
fprintf('\n The RMSE value of No AA is %0.6f \n', sqrt(mse_no_aa));

% PSNR, Bigger means it's good
fprintf('\nPSNR and SNR, Bigger means it''s good');
fprintf('\n The Peak-SNR value of Temporal is %0.6f', peaksnr);
fprintf('\n The SNR value of Temporal is %0.6f', snr);
fprintf('\n The Peak-SNR value of No AA is %0.6f', peaksnr_no_aa);
fprintf('\n The SNR value of No AA is %0.6f \n', snr_no_aa);

% SSIM, Close to one means it's good
fprintf('\nSSIM, Close to one means it''s good');
fprintf('\n The SSIM value of Temporal is %0.6f',ssimval);
fprintf('\n The SSIM value of No AA is %0.6f \n',ssimval_no_aa);

diary off;

% Extra Results
diary(results_extra_filename);
% NIQUE, Lower values of score reflect better perceptual quality of image
% with respect to the input MATLAB model
fprintf('\nNIQUE, Lower values of score means it''s good');
fprintf('\n The NIQE score of Temporal is %0.6f', niqeI);
fprintf('\n The NIQE score of Reference is %0.6f', niqeRef);
fprintf('\n The NIQE score of No AA is %0.6f', niqeI_no_aa);
fprintf('\n The NIQE score delta between Temporal and Reference is %+0.6f', ... 
    niqeI - niqeRef);
fprintf('\n The NIQE score delta between Temporal and No AA is %+0.6f \n', ... 
    niqeI - niqeI_no_aa);

% BRISQUE, Lower values of score reflect better perceptual quality of image
% with respect to the input MATLAB model
fprintf('\nBRISQUE, Lower values of score means it''s good');
fprintf('\n The BRISQUE score of Temporal is %0.6f', brisqueI);
fprintf('\n The BRISQUE score of Reference is %0.6f', brisqueRef);
fprintf('\n The BRISQUE score of No AA is %0.6f', brisqueI_no_aa);
fprintf('\n The BRISQUE score delta between Temporal and Reference is %+0.6f', ...
    brisqueI - brisqueRef);
fprintf('\n The BRISQUE score delta between Temporal and No AA is %+0.6f \n', ...
    brisqueI - brisqueI_no_aa);

diary off;

% SSIM Maps
figure('Name','SSIM Index Map of Temporal'), imshow(ssimmap);
title(sprintf('The SSIM Index Map of Temporal- Mean ssim Value is %0.6f', ...
    ssimval));
savefig(fullfile(name, strcat(name, '_ssim_map_temporal.fig')));
saveas(gcf, fullfile(name, strcat(name, '_ssim_map_temporal.png')));
close(gcf);

figure('Name','SSIM Index Map of No AA'), imshow(ssimmap_no_aa);
title(sprintf('The SSIM Index Map of No AA - Mean ssim Value is %0.6f', ...
    ssimval_no_aa));
savefig(fullfile(name, strcat(name, '_ssim_map_no_aa.fig')));
saveas(gcf, fullfile(name, strcat(name, '_ssim_map_no_aa.png')));
close(gcf);

fprintf('The test %s is Complete\n', ...
    name);
code = 0;
end

