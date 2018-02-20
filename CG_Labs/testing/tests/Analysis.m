% Image Analysis by Christian Oliveros 2018/01/23
% Last Revision 2018/02/01
% For his master thesis
clearvars; clc;

current_directory = dir();
current_directory(~[current_directory.isdir]) = [];  %remove non-directories
tf = ismember( {current_directory.name}, {'.', '..'});
current_directory(tf) = [];  %remove current and parent directory.

log_file = fullfile('.', 'tests_log.txt');

diary(log_file);

fprintf('Timestamp(dd/mm/yyyy): %s\n', datestr(now,'dd/mm/yyyy HH:MM:SS.FFF'))
fprintf('Amount of Tests found: %d\n', length(current_directory));
fprintf('Starting Tests\n\n');

for i = 1:length(current_directory)
	fprintf('Working on test: %s\n', current_directory(i).name);
	diary off;

	ret_code = analysis_function(current_directory(i).name);

	diary(log_file);
	fprintf('Return Code of the Tests: %d\n\n', ...
		ret_code);
end

fprintf('Tests Done\n');
fprintf('\n-----------------------------------------------------------------\n');
diary off;
    