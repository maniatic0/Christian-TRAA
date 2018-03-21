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

ghosting_test_filename = fullfile(name, strcat(name, '_log_gt.txt'));

time_stamp = datestr(now,'dd/mm/yyyy HH:MM:SS.FFF');

% Ghosting Tests
if exist(ghosting_test_filename, 'file') == 2
    
    % Get the amount of tests
    fileID = fopen(ghosting_test_filename,'r');
    formatSpec = '%d';
    test_amount = fscanf(fileID,formatSpec);
    fclose(fileID);
    
    % Save text to diary
    diary(results_filename);
    fprintf('Name: %s\n', name);
    fprintf('Timestamp(dd/mm/yyyy): %s\n', time_stamp);
    fprintf('Ghosting Test Found: %s\n', ghosting_test_filename);
    fprintf('Ghosting Test Amount: %d\n', test_amount);
    diary off;
    
    % Extra Results
    diary(results_extra_filename);
    fprintf('Name: %s\n', name);
    fprintf('Timestamp(dd/mm/yyyy): %s\n', time_stamp);
    fprintf('Ghosting Test Found: %s\n', ghosting_test_filename);
    fprintf('Ghosting Test Amount: %d\n', test_amount);
    diary off;
    
    for i=0:test_amount-1
        % Save text to diary
        test_n = sprintf('%04d',i);
        
        improved_filename = fullfile(name, ...
            strcat(name, sprintf('_both_%s_improved.png',test_n)));
        no_improved_filename = fullfile(name, ...
            strcat(name, sprintf('_both_%s_no_improved.png',test_n)));
        ground_truth_filename = fullfile(name, ...
            strcat(name, sprintf('_both_%s_truth.png',test_n)));
        
        improved = imread(improved_filename);
        no_improved = imread(no_improved_filename);
        ground_truth = imread(ground_truth_filename);
        
        % Test againts Master Thesis Temporal result
        [ mse, peaksnr, snr, ssimval, ssimmap, ...
            niqeI, niqeRef, brisqueI, brisqueRef ] ...
            = Test_Files(improved, ground_truth);
        
        % Test againts Uncharted Temporal result
        [ mse_no, peaksnr_no, snr_no, ssimval_no, ssimmap_no, ...
            niqeI_no, ~, brisqueI_no, ~ ] ...
            = Test_Files(no_improved, ground_truth);
        
        diary(results_filename);
        fprintf('\n\nTest Number: %s\n', test_n);
        fprintf('Ground Truth: %s\n', ground_truth_filename);
        fprintf('Master Thesis Temporal: %s\n', improved_filename);
        fprintf('Uncharted Temporal: %s\n', no_improved_filename);
        
        % MSE, Close to zero means it's good
        fprintf('\nMSE and RMSE, Close to zero means it''s good');
        fprintf('\n The MSE value of Master Thesis Temporal is %0.6f', mse);
        fprintf('\n The RMSE value of Master Thesis Temporal is %0.6f', sqrt(mse));
        fprintf('\n The MSE value of Uncharted Temporal is %0.6f', mse_no);
        fprintf('\n The RMSE value of Uncharted Temporal is %0.6f \n', sqrt(mse_no));

        % PSNR, Bigger means it's good
        fprintf('\nPSNR and SNR, Bigger means it''s good');
        fprintf('\n The Peak-SNR value of Master Thesis Temporal is %0.6f', peaksnr);
        fprintf('\n The SNR value of Master Thesis Temporal is %0.6f', snr);
        fprintf('\n The Peak-SNR value of Uncharted Temporal is %0.6f', peaksnr_no);
        fprintf('\n The SNR value of Uncharted Temporal is %0.6f \n', snr_no);

        % SSIM, Close to one means it's good
        fprintf('\nSSIM, Close to one means it''s good');
        fprintf('\n The SSIM value of Master Thesis Temporal is %0.6f',ssimval);
        fprintf('\n The SSIM value of Uncharted Temporal is %0.6f \n',ssimval_no);
        
        diary off;
        
        
        % Extra Results
        diary(results_extra_filename);
        fprintf('\n\nTest Number: %s\n', test_n);
        fprintf('Ground Truth: %s\n', ground_truth_filename);
        fprintf('Master Thesis Temporal: %s\n', improved_filename);
        fprintf('Temporal Not Improved: %s\n', no_improved_filename);

        % NIQUE, Lower values of score reflect better perceptual quality of image
        % with respect to the input MATLAB model
        fprintf('\nNIQUE, Lower values of score means it''s good');
        fprintf('\n The NIQE score of Master Thesis Temporal is %0.6f', niqeI);
        fprintf('\n The NIQE score of Reference is %0.6f', niqeRef);
        fprintf('\n The NIQE score of Uncharted Temporal is %0.6f', niqeI_no);
        fprintf('\n The NIQE score delta between Master Thesis Temporal and Reference is %+0.6f', ... 
            niqeI - niqeRef);
        fprintf('\n The NIQE score delta between Master Thesis Temporal and Uncharted Temporal is %+0.6f \n', ... 
            niqeI - niqeI_no);

        % BRISQUE, Lower values of score reflect better perceptual quality of image
        % with respect to the input MATLAB model
        fprintf('\nBRISQUE, Lower values of score means it''s good');
        fprintf('\n The BRISQUE score of Master Thesis Temporal is %0.6f', brisqueI);
        fprintf('\n The BRISQUE score of Reference is %0.6f', brisqueRef);
        fprintf('\n The BRISQUE score of Uncharted Temporal is %0.6f', brisqueI_no);
        fprintf('\n The BRISQUE score delta between Master Thesis Temporal and Reference is %+0.6f', ...
            brisqueI - brisqueRef);
        fprintf('\n The BRISQUE score delta between Master Thesis Temporal and Uncharted Temporal is %+0.6f \n', ...
            brisqueI - brisqueI_no);

        diary off;
        
        % SSIM Maps
        figure('Name','SSIM Index Map of Master Thesis Temporal'), imshow(ssimmap);
        title(sprintf('The SSIM Index Map of Master Thesis Temporal - Mean ssim Value is %0.6f', ...
            ssimval));
        savefig(fullfile(name, strcat(name, sprintf('_ssim_map_%s_master_thesis_temporal.fig',test_n))));
        saveas(gcf, fullfile(name, strcat(name, sprintf('_ssim_map_%s_master_thesis_temporal.png',test_n))));
        close(gcf);

        figure('Name','SSIM Index Map of Uncharted Temporal'), imshow(ssimmap_no);
        title(sprintf('The SSIM Index Map of Uncharted Temporal - Mean ssim Value is %0.6f', ...
            ssimval_no));
        savefig(fullfile(name, strcat(name, sprintf('_ssim_map_%s_uncharted_temporal.fig',test_n))));
        saveas(gcf, fullfile(name, strcat(name, sprintf('_ssim_map_%s_uncharted_temporal.png',test_n))));
        close(gcf);
        
    end


    fprintf('The test %s is Complete\n', ...
        name);
    code = 0;
    return; 
end



% Normal Accumulation Buffer Test
temporal_filename = fullfile(name, strcat(name, '_temporal.png'));
ground_truth_filename = fullfile(name, strcat(name, '_ground_truth.png'));
no_aa_filename = fullfile(name, strcat(name, '_no_aa.png'));
fxaa_filename = fullfile(name, strcat(name, '_fxaa.png'));

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

if exist(fxaa_filename, 'file') ~= 2 
    fprintf('The no FXAA test file was not found: %s\n', ...
        fxaa_filename);
    code = -1;
    return;
end

temporal = imread(temporal_filename);
ground_truth = imread(ground_truth_filename);
no_aa = imread(no_aa_filename);
fxaa = imread(fxaa_filename);

% Test againts temporal result
[ mse, peaksnr, snr, ssimval, ssimmap, ...
    niqeI, niqeRef, brisqueI, brisqueRef ] ...
    = Test_Files(temporal, ground_truth);

% Test againts no AA result
[ mse_no_aa, peaksnr_no_aa, snr_no_aa, ssimval_no_aa, ssimmap_no_aa, ...
    niqeI_no_aa, ~ , brisqueI_no_aa, ~ ] ...
    = Test_Files(no_aa, ground_truth);

% Test againts FXAA result
[ mse_fxaa, peaksnr_fxaa, snr_fxaa, ssimval_fxaa, ssimmap_fxaa, ...
    niqeI_fxaa, ~ , brisqueI_fxaa, ~ ] ...
    = Test_Files(fxaa, ground_truth);

% Main Results
% Save text to diary
diary(results_filename);
fprintf('Name: %s\n', name);
fprintf('Timestamp(dd/mm/yyyy): %s\n', time_stamp);

% MSE, Close to zero means it's good
fprintf('\nMSE and RMSE, Close to zero means it''s good');
fprintf('\n The MSE value of Temporal is %0.6f', mse);
fprintf('\n The RMSD value of Temporal is %0.6f', sqrt(mse));
fprintf('\n The MSE value of No AA is %0.6f', mse_no_aa);
fprintf('\n The RMSD value of No AA is %0.6f', sqrt(mse_no_aa));
fprintf('\n The MSE value of FXAA is %0.6f', mse_fxaa);
fprintf('\n The RMSD value of FXAA is %0.6f \n', sqrt(mse_fxaa));

% PSNR, Bigger means it's good
fprintf('\nPSNR and SNR, Bigger means it''s good');
fprintf('\n The Peak-SNR value of Temporal is %0.6f', peaksnr);
fprintf('\n The SNR value of Temporal is %0.6f', snr);
fprintf('\n The Peak-SNR value of No AA is %0.6f', peaksnr_no_aa);
fprintf('\n The SNR value of No AA is %0.6f', snr_no_aa);
fprintf('\n The Peak-SNR value of FXAA is %0.6f', peaksnr_fxaa);
fprintf('\n The SNR value of FXAA is %0.6f \n', snr_fxaa);

% SSIM, Close to one means it's good
fprintf('\nSSIM, Close to one means it''s good');
fprintf('\n The SSIM value of Temporal is %0.6f', ssimval);
fprintf('\n The SSIM value of No AA is %0.6f', ssimval_no_aa);
fprintf('\n The SSIM value of FXAA is %0.6f \n', ssimval_fxaa);

diary off;

% Extra Results
diary(results_extra_filename);
fprintf('Name: %s\n', name);
fprintf('Timestamp(dd/mm/yyyy): %s\n', time_stamp);

% NIQUE, Lower values of score reflect better perceptual quality of image
% with respect to the input MATLAB model
fprintf('\nNIQUE, Lower values of score means it''s good');
fprintf('\n The NIQE score of Temporal is %0.6f', niqeI);
fprintf('\n The NIQE score of Reference is %0.6f', niqeRef);
fprintf('\n The NIQE score of No AA is %0.6f', niqeI_no_aa);
fprintf('\n The NIQE score of FXAA is %0.6f', niqeI_fxaa);
fprintf('\n The NIQE score delta between Temporal and Reference is %+0.6f', ... 
    niqeI - niqeRef);
fprintf('\n The NIQE score delta between Temporal and No AA is %+0.6f', ... 
    niqeI - niqeI_no_aa);
fprintf('\n The NIQE score delta between Temporal and FXAA is %+0.6f \n', ... 
    niqeI - niqeI_fxaa);

% BRISQUE, Lower values of score reflect better perceptual quality of image
% with respect to the input MATLAB model
fprintf('\nBRISQUE, Lower values of score means it''s good');
fprintf('\n The BRISQUE score of Temporal is %0.6f', brisqueI);
fprintf('\n The BRISQUE score of Reference is %0.6f', brisqueRef);
fprintf('\n The BRISQUE score of No AA is %0.6f', brisqueI_no_aa);
fprintf('\n The BRISQUE score of FXAA is %0.6f', brisqueI_fxaa);
fprintf('\n The BRISQUE score delta between Temporal and Reference is %+0.6f', ...
    brisqueI - brisqueRef);
fprintf('\n The BRISQUE score delta between Temporal and No AA is %+0.6f', ...
    brisqueI - brisqueI_no_aa);
fprintf('\n The BRISQUE score delta between Temporal and FXAA is %+0.6f \n', ...
    brisqueI - brisqueI_fxaa);

diary off;

% SSIM Maps
figure('Name','SSIM Index Map of Temporal'), imshow(ssimmap);
title(sprintf('The SSIM Index Map of Temporal - Mean ssim Value is %0.6f', ...
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

figure('Name','SSIM Index Map of FXAA'), imshow(ssimmap_fxaa);
title(sprintf('The SSIM Index Map of FXAA - Mean ssim Value is %0.6f', ...
    ssimval_fxaa));
savefig(fullfile(name, strcat(name, '_ssim_map_fxaa.fig')));
saveas(gcf, fullfile(name, strcat(name, '_ssim_map_fxaa.png')));
close(gcf);

fprintf('The test %s is Complete\n', ...
    name);
code = 0;
end

