% Image Analysis by Christian Oliveros 2018/01/23
% For his master thesis
clearvars;

name = input('Please Write the start of the name of the test files: ','s');

temporal_filename = strcat(name, '_temporal.png');
ground_truth_filename = strcat(name, '_ground_truth.png');

if exist(temporal_filename, 'file') ~= 2 
    print(strcat('The temporal test file was not found: ', ...
        temporal_filename));
    return; 
end

if exist(ground_truth_filename, 'file') ~= 2 
    print(strcat('The ground truth test file was not found: ', ...
        ground_truth_filename));
    return;
end

temporal = imread(temporal_filename);
ground_truth = imread(ground_truth_filename);

[ mse, peaksnr, snr, ssimval, ssimmap, ...
    niqeI, niqeRef, brisqueI, brisqueRef ] ...
    = Test_Files(temporal, ground_truth);

% MSE, Close to zero means it's good
fprintf('\nMSE, Close to zero means it''s good');
fprintf('\n The MSE value is %0.4f \n', mse);

% PSNR, Close to zero means it's good
fprintf('\nPSNR, Close to zero means it''s good');
fprintf('\n The Peak-SNR value is %0.4f', peaksnr);
fprintf('\n The SNR value is %0.4f \n', snr);

% SSIM, Close to one means it's good
fprintf('\nSSIM, Close to one means it''s good');
fprintf('\n The SSIM value is %0.4f \n',ssimval);
title(sprintf(' The SSIM Index Map - Mean ssim Value is %0.4f',ssimval));

% NIQUE, Lower values of score reflect better perceptual quality of image
% with respect to the input MATLAB model
fprintf('\nNIQUE, Lower values of score means it''s good');
fprintf('\n The NIQE score is %0.4f', niqeI);
fprintf('\n The NIQE reference score is %0.4f', niqeRef);
fprintf('\n The NIQE score delta is %+0.4f \n', niqeI - niqeRef);

% BRISQUE, Lower values of score reflect better perceptual quality of image
% with respect to the input MATLAB model
fprintf('\nBRISQUE, Lower values of score means it''s good');
fprintf('\n The BRISQUE score is %0.4f', brisqueI);
fprintf('\n The BRISQUE reference score is %0.4f', brisqueRef);
fprintf('\n The BRISQUE score delta is %+0.4f \n', brisqueI - brisqueRef);


