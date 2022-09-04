// DarkHelp then pulls in OpenCV and much more, so this keeps the headers simple.
#include <DarkHelp.hpp>

#include "main.h"
#include <fstream> // needed for Ubuntu 18.04



const std::string darkplate_configuration	= "own_darkmark_dataset.cfg";
const std::string darkplate_best_weights	= "own_darkmark_dataset_best.weights";
const std::string darkplate_names			= "own_darkmark_dataset.names";
const size_t class_plate					= 0;
const auto font_face						= cv::FONT_HERSHEY_PLAIN;
const auto font_border						= 10.0;
const auto font_scale						= 3.5;
const auto font_thickness					= 2;
cv::Size network_size;

std::vector<std::string> listOfDetectedPlates;


void draw_label(const std::string & txt, cv::Mat & mat, const cv::Point & tl, const double factor) // factor default 1.0
{
	const auto border		= factor * font_border;
	const auto scale		= factor * font_scale;
	const auto thickness	= static_cast<int>(std::max(1.0, factor * font_thickness));

	const cv::Size text_size = cv::getTextSize(txt, font_face, scale, thickness, nullptr);
	cv::Rect r(tl.x, tl.y - text_size.height - (border * 3), text_size.width + border * 2, text_size.height + border * 2);

	if (r.x + r.width	> mat.cols)	{	r.x = mat.cols - r.width	- border;	}
	if (r.y + r.height	> mat.rows)	{	r.y = mat.rows - r.height	- border;	}
	if (r.x < 0)					{	r.x = 0;								}
	if (r.y < 0)					{	r.y = 0;								}

	// lighten a box into which we'll write some text
	cv::Mat tmp;
	mat(r).convertTo(tmp, -1, 1, 125.0);

	const cv::Point point(border, tmp.rows - border);
	cv::putText(tmp, txt, point, font_face, scale, {0, 0, 0}, thickness, cv::LINE_AA);

	// copy the box and text back into the image
	tmp.copyTo(mat(r));

	return;
}


/// This is the 2nd stage detection.  By the time this is called, we have a smaller Roi, we no longer have the full frame.
void process_plate(DarkHelp::NN & nn, cv::Mat & plate, cv::Mat & output)
{
	auto results = nn.predict(plate);
	if (results.empty())
	{
		// nothing we can do with this image since no license plate was found
//		std::cout << "-> failed find a plate in this RoI" << std::endl;
		return;
	}

	// sort the results from left-to-right based on the mid-x point of each detected object
	std::sort(results.begin(), results.end(),
			[](const DarkHelp::PredictionResult & lhs, const DarkHelp::PredictionResult & rhs)
			{
				// put the "license plate" class first so the characters are drawn overtop of this class
				if (lhs.best_class == class_plate)	return true;
				if (rhs.best_class == class_plate)	return false;

				// otherwise, sort by the horizontal coordinate
				// (this obviously only works with license plates that consist of a single row of characters)
				return lhs.original_point.x < rhs.original_point.x;
			});

//	std::cout << "-> results: " << results << std::endl;

	cv::Point tl = results[0].rect.tl();

	// go over the plate class-by-class and build up what we think the license plate might be
	std::string license_plate;
	double probability = 0.0;
	for (const auto & prediction : results)
	{
		if (prediction.rect.x < tl.x) tl.x = prediction.rect.x;
		if (prediction.rect.y < tl.y) tl.y = prediction.rect.y;

		probability += prediction.best_probability;
		if (prediction.best_class != class_plate)
		{
			license_plate += nn.names[prediction.best_class];
		}
	}

	// store the sorted results back in DarkHelp so the annotations are drawn with the license plate first
	nn.prediction_results = results;
	cv::Mat mat = nn.annotate();

	if (license_plate.empty() == false)
	{
		const std::string label = license_plate + " [" + std::to_string((size_t)std::round(100.0 * probability / results.size())) + "%]";
		std::cout << "-> license plate: " << label << std::endl;
		listOfDetectedPlates.push_back(label);

		draw_label(license_plate /* label */, mat, tl);
	}

	// copy the annotated RoI back into the output image to be used when writing the video
	mat.copyTo(output);

	return;
}


/** Process a single license plate located within the given prediction.
 * This means we build a RoI and apply it the rectangle to both the frame and the output image.
 */
void process_plate(DarkHelp::NN & nn, cv::Mat & frame, const DarkHelp::PredictionResult & prediction, cv::Mat & output_frame)
{
	cv::Rect roi = prediction.rect;

	if (roi.width < 1 or roi.height < 1)
	{
		std::cout << "-> ignoring impossibly small plate (x=" << roi.x << " y=" << roi.y << " w=" << roi.width << " h=" << roi.height << ")" << std::endl;
		return;
	}

	// increase the RoI to match the network dimensions, but stay within the bounds of the frame
	if (roi.width >= network_size.width or roi.height >= network_size.height)
	{
		// something is wrong with this plate, since it seems to be the same size or bigger than the original frame size!
		std::cout << "-> ignoring too-big plate (x=" << roi.x << " y=" << roi.y << " w=" << roi.width << " h=" << roi.height << ")" << std::endl;
		return;
	}

	const double dx = 0.5 * (network_size.width		- roi.width	);
	const double dy = 0.5 * (network_size.height	- roi.height);

	roi.x		-= std::floor(dx);
	roi.y		-= std::floor(dy);
	roi.width	+= std::ceil(dx * 2.0);
	roi.height	+= std::ceil(dy * 2.0);

	// check all the edges and reposition the RoI if necessary
	if (roi.x < 0)							roi.x = 0;
	if (roi.y < 0)							roi.y = 0;
	if (roi.x + roi.width	> frame.cols)	roi.x = frame.cols - roi.width;
	if (roi.y + roi.height	> frame.rows)	roi.y = frame.rows - roi.height;

	#if 0
	std::cout	<< "-> plate found: " << prediction << std::endl
				<< "-> roi: x=" << roi.x << " y=" << roi.y << " w=" << roi.width << " h=" << roi.height << std::endl;
	#endif

	// the RoI should now be the same size as the network dimensions, and all edges should be valid
	cv::Mat plate = frame(roi);
	cv::Mat output = output_frame(roi);
	process_plate(nn, plate, output);

	return;
}


cv::Mat process_frame(DarkHelp::NN & nn, cv::Mat & frame)
{
	cv::Mat output_frame = frame.clone();

	// we need to find all the license plates in the image
	auto result = nn.predict(frame);
	for (const auto & prediction : result)
	{
		// at this stage we're only interested in the "license plate" class, ignore everything else
		if (prediction.best_class == class_plate)
		{
			process_plate(nn, frame, prediction, output_frame);
		}
	}

	return output_frame;
}


void process_File(DarkHelp::NN & nn, const std::string & filename)
{
	std::cout << "Processing video file \"" << filename << "\"" << std::endl;

	std::string basename = filename;
	size_t p = basename.rfind("/");
	if (p != std::string::npos)
	{
		basename.erase(0, p + 1);
	}
	p = basename.rfind(".");
	if (p != std::string::npos)
	{
		basename.erase(p);
	}

	cv::VideoCapture cap(filename);
	if (cap.isOpened() == false)
	{
		std::cout << "ERROR: \"" << filename << "\" is not a valid video file, or perhaps does not exist?" << std::endl;
		return;
	}

	const double width		= cap.get(cv::VideoCaptureProperties::CAP_PROP_FRAME_WIDTH	);
	const double height		= cap.get(cv::VideoCaptureProperties::CAP_PROP_FRAME_HEIGHT	);
	const double frames		= cap.get(cv::VideoCaptureProperties::CAP_PROP_FRAME_COUNT	);
	const double fps		= cap.get(cv::VideoCaptureProperties::CAP_PROP_FPS			);
	const size_t round_fps	= std::round(fps);

	std::cout	<< "-> " << static_cast<size_t>(width) << " x " << static_cast<size_t>(height) << " @ " << fps << " FPS" << std::endl
				<< "-> " << frames << " frames (" << static_cast<size_t>(std::round(frames / fps)) << " seconds)" << std::endl;

	if (width < network_size.width or height < network_size.height)
	{
		std::cout << "ERROR: \"" << filename << "\" [" << width << " x " << height << "] is smaller than the network size " << network_size << "!" << std::endl;
		return;
	}

	cv::VideoWriter output;
	output.open("output/" + basename + "_output.mp4", cv::VideoWriter::fourcc('m', 'p', '4', 'v'), fps, cv::Size(width, height)); //FIXME: "output/" + at the beggining causes saving saving the output video to fail, which is good for now!

	size_t frame_counter = 0;
	while (true)
	{
		const auto t1 = std::chrono::high_resolution_clock::now();

		cv::Mat frame;
		cap >> frame;
		if (frame.empty())
		{
			break;
		}

		if (frame_counter % round_fps == 0)
		{
			std::cout << "\r-> frame #" << frame_counter << " (" << std::round(100 * frame_counter / frames) << "%)" << std::flush;
		}

		auto output_frame = process_frame(nn, frame);

		const auto t2 = std::chrono::high_resolution_clock::now();

		// "steal" the duration format function in DarkHelp
		draw_label(DarkHelp::duration_string(t2 - t1), output_frame, cv::Point(0, 0), 0.5);

		//cv::imshow(basename, output_frame);
		cv::waitKey(5);

		output.write(output_frame);

		frame_counter ++;
	}
	std::cout << "\r-> done processing " << frame_counter << " frames from " << filename << std::endl;

	return;
}

std::vector<std::string> getListOfRecognitionsFromVideo(std::string& filenameToProcess)
{
	try
	{
		DarkHelp::NN nn;

		// first thing we need to do is find the neural network
		bool initialization_done = false;
		for (const auto path : { "./", "../", "../../", "nn/", "../nn/", "../../nn/", "C:/Users/olokos/Documents/Projekty/EngineersThesisOCR/resources/nn/" })
		{
			const auto fn = path + darkplate_configuration;
			std::cout << "Looking for " << fn << std::endl;
			std::ifstream ifs(fn);
			if (ifs.is_open())
			{
				const DarkHelp::EDriver driver = DarkHelp::EDriver::kDarknet;

				ifs.close();
				std::cout << "Found neural network: " << fn << std::endl;
				const std::string cfg = fn;
				const std::string names = path + darkplate_names;
				const std::string weights = path + darkplate_best_weights;
				nn.init(cfg, weights, names, true, driver);
				nn.config.enable_debug = false;
				nn.config.annotation_auto_hide_labels = false;
				nn.config.annotation_include_duration = false;
				nn.config.annotation_include_timestamp = false;
				nn.config.enable_tiles = false;
				nn.config.combine_tile_predictions = true;
				nn.config.include_all_names = true;
				nn.config.names_include_percentage = true;
				nn.config.threshold = 0.25;
				nn.config.sort_predictions = DarkHelp::ESort::kUnsorted;
				initialization_done = true;
				break;
			}
		}
		if (initialization_done == false)
		{
			throw std::runtime_error("failed to find the neural network " + darkplate_configuration);
		}

		// remember the size of the network, since we'll need to crop plates to this exact size
		network_size = nn.network_size();

		//for (int idx = 1; idx < argv.length(); idx++)
		//{ 

		process_File(nn, filenameToProcess); //Only single file!
		//}
	}
	catch (const std::exception& e)
	{
		std::cout << std::endl << "ERROR: " << e.what() << std::endl;
		return std::vector<std::string>{"Error code 1!", e.what()}; //Return the error as error code + stack trace in list
	}
	catch (...)
	{
		std::cout << std::endl << "ERROR: unknown exception" << std::endl;
		return std::vector<std::string>{"Unknown error!", "Code 2"}; //Return the error as error code + stack trace in list
	}

	return listOfDetectedPlates;
}


int main(int argc, char *argv[])
{
	try
	{
		DarkHelp::NN nn;

		// first thing we need to do is find the neural network
		bool initialization_done = false;
		for (const auto path : {"./", "../", "../../", "nn/", "../nn/", "../../nn/", "C:/Users/olokos/Documents/Projekty/EngineersThesisOCR/resources/nn/"})
		{
			const auto fn = path + darkplate_configuration;
			std::cout << "Looking for " << fn << std::endl;
			std::ifstream ifs(fn);
			if (ifs.is_open())
			{
				const DarkHelp::EDriver driver = DarkHelp::EDriver::kDarknet;

				ifs.close();
				std::cout << "Found neural network: " << fn << std::endl;
				const std::string cfg		= fn;
				const std::string names		= path + darkplate_names;
				const std::string weights	= path + darkplate_best_weights;
				nn.init(cfg, weights, names, true, driver);
				nn.config.enable_debug					= false;
				nn.config.annotation_auto_hide_labels	= false;
				nn.config.annotation_include_duration	= false;
				nn.config.annotation_include_timestamp	= false;
				nn.config.enable_tiles					= false;
				nn.config.combine_tile_predictions		= true;
				nn.config.include_all_names				= true;
				nn.config.names_include_percentage		= true;
				nn.config.threshold						= 0.25;
				nn.config.sort_predictions				= DarkHelp::ESort::kUnsorted;
				initialization_done						= true;
				break;
			}
		}
		if (initialization_done == false)
		{
			throw std::runtime_error("failed to find the neural network " + darkplate_configuration);
		}

		// remember the size of the network, since we'll need to crop plates to this exact size
		network_size = nn.network_size();

		for (int idx = 1; idx < argc; idx ++)
		{
			process_File(nn, argv[idx]);
		}
	}
	catch (const std::exception & e)
	{
		std::cout << std::endl << "ERROR: " << e.what() << std::endl;
		return 1;
	}
	catch (...)
	{
		std::cout << std::endl << "ERROR: unknown exception" << std::endl;
		return 2;
	}

	return 0;
}
